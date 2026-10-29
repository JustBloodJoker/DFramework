#include "pch.h"

#include <System/Shadows/ShadowSystem.h>

#include <Config/NeuralShadowConfig.h>
#include <Component/Shadow/ShadowComponent.h>
#include <Component/Light/LightComponent.h>
#include <Component/RenderObject/MeshComponent.h>
#include <Entity/Core/ComponentHolder.h>
#include <World/World.h>
#include <MainRenderer/MainRenderer.h>

void ShadowSystem::AfterConstruction() {
	RefreshUserSettings();
}

std::filesystem::path ShadowSystem::SelectedNeuralModel() const {
	return FDWConfig::NeuralShadows::ModelPath;
}

void ShadowSystem::ProcessNotify(NRenderSystemNotifyType type) {
	if (type == NRenderSystemNotifyType::Shadow ||
		type == NRenderSystemNotifyType::Light ||
		type == NRenderSystemNotifyType::LightUpdateData) {

		m_bNeedsRefresh.store(true, std::memory_order_release);

	}

	if (m_pAtlasBackend) m_pAtlasBackend->ProcessNotify(type);
}

void ShadowSystem::InitializeBackend() {
	if (m_pAtlasBackend) return;

	m_pAtlasBackend = std::make_unique<ShadowAtlasRenderer>(*this);
	m_pAtlasBackend->Initialize();
}

void ShadowSystem::EnsureNeuralWorkGenerator() {
	if (!m_pNeuralWorkGenerator) {
		m_pNeuralWorkGenerator = std::make_unique<NeuralShadowWorkGenerator>(*this);
		m_pNeuralWorkGenerator->Initialize();
	}
}

void ShadowSystem::EnsureNeuralBaseline() {
	if (m_pNeuralBaselinePass) return;

	m_pNeuralBaselinePass = std::make_unique<NeuralShadowBaselinePass>(*this);
	m_pNeuralBaselinePass->Initialize();

	if (m_pAtlasBackend && m_pNormalInput) {
		m_pNeuralBaselinePass->SetInputResources(m_pAtlasBackend->GetShadowAtlas(), m_pNormalInput, GetDevice());
	}
}

NeuralShadowInferencePass::State ShadowSystem::EnsureNeuralRuntimeInitialized() {
	if (!m_pNeuralShadowInferencePass) m_pNeuralShadowInferencePass = std::make_unique<NeuralShadowInferencePass>(*this);

	if (m_pNormalInput) m_pNeuralShadowInferencePass->SetInputResources(m_pNormalInput, GetDevice());

	return m_pNeuralShadowInferencePass->Initialize(SelectedNeuralModel());
}

bool ShadowSystem::SelectNeuralRuntimeAfterGpuIdle() {
	m_bNeuralInitializationInProgress.store(true, std::memory_order_release);
	m_bNeuralSelected = true;

	const auto state = EnsureNeuralRuntimeInitialized();
	m_bNeuralInitializationInProgress.store(false, std::memory_order_release);

	return state == NeuralShadowInferencePass::State::Ready;
}


bool ShadowSystem::IsNeuralRuntimeSelected() const noexcept {
	return m_bNeuralSelected;
}

NeuralShadowRuntimeState ShadowSystem::GetNeuralRuntimeState(bool rtSupported) const {
	NeuralShadowRuntimeState result{};
	result.Selected = IsNeuralRuntimeSelected();
	if (!rtSupported) {
		result.Status = NeuralShadowRuntimeStatus::Unavailable;
		result.Reason = "DXR is unavailable on the active adapter.";
		return result;
	}
	const auto graph = SelectedNeuralModel();

	if (!std::filesystem::is_regular_file(graph)) {
		result.Status = NeuralShadowRuntimeStatus::Unavailable;
		result.Reason = "Frozen neural-shadow ONNX model is missing: " + graph.string();
		return result;
	}
	if (m_bNeuralInitializationInProgress.load(std::memory_order_acquire)) {
		result.Status = NeuralShadowRuntimeStatus::Loading;
		result.Reason = "Initializing ONNX Runtime and the DirectML execution provider.";
		return result;
	}
	if (!result.Selected || !m_pNeuralShadowInferencePass) {
		result.Status = NeuralShadowRuntimeStatus::Ready;
		result.Reason = "The frozen neural-shadow model is available and will load when selected.";
		return result;
	}

	const auto runtimeState = m_pNeuralShadowInferencePass->GetRuntimeState();
	result.Loaded = runtimeState.Status == NeuralShadowInferencePass::State::Ready;

	switch (runtimeState.Status) {
	case NeuralShadowInferencePass::State::Ready:
		result.Status = NeuralShadowRuntimeStatus::Ready;
		result.Reason = "The neural-shadow model is loaded through ONNX Runtime + DirectML.";
		break;
	case NeuralShadowInferencePass::State::Unavailable:
		result.Status = NeuralShadowRuntimeStatus::Unavailable;
		result.Reason = runtimeState.LastError.empty() ? "The neural-shadow runtime is unavailable." : runtimeState.LastError;
		break;
	case NeuralShadowInferencePass::State::Error:
		result.Status = NeuralShadowRuntimeStatus::Error;
		result.Reason = runtimeState.LastError.empty() ? "Neural-shadow runtime initialization failed." : runtimeState.LastError;
		break;
	case NeuralShadowInferencePass::State::Uninitialized:
	default:
		result.Status = NeuralShadowRuntimeStatus::Loading;
		result.Reason = "Neural-shadow runtime initialization is pending.";
		break;
	}
	return result;
}

bool ShadowSystem::IsNeuralInferenceReady() const {
	return m_pNeuralShadowInferencePass && m_pNeuralShadowInferencePass->IsReady();
}

void ShadowSystem::RefreshUserSettings() {
	if (!m_bNeedsRefresh.exchange(false, std::memory_order_acq_rel) || !GetWorld()) return;

	auto components = GetWorld()->GetAllComponentsOfType<LightComponent>();

	std::vector<ShadowComponentData> settings;
	settings.reserve(components.size());
	for (auto* light : components) {
		if (!light || !light->IsActive()) continue;

		ShadowComponentData data{};
		auto* owner = light->GetOwner();
		auto* shadow = owner ? owner->GetComponent<ShadowComponent>() : nullptr;
		if (shadow) {
			data = shadow->GetShadowComponentData();
			data.CastsShadows = data.CastsShadows && shadow->IsActive();
		}

		settings.push_back(data);
	}

	std::lock_guard<std::mutex> lock(m_xSettingsMutex);
	m_vUserSettings = std::move(settings);
}

std::vector<ShadowComponentData> ShadowSystem::GetUserSettingsSnapshot() const {
	const_cast<ShadowSystem*>(this)->RefreshUserSettings();
	std::lock_guard<std::mutex> lock(m_xSettingsMutex);
	return m_vUserSettings;
}

std::vector<ShadowComponentData> ShadowSystem::GetShadowLightSettings() const {
	return GetUserSettingsSnapshot();
}

ShadowFrameContext ShadowSystem::BuildFrameContext(std::uint64_t frameIndex, ShadowMode requestedMode, ShadowBackendAvailability availability, std::uint32_t lightCount) const {
	auto context = m_xRouter.Resolve(frameIndex, requestedMode, availability, lightCount);

	std::lock_guard<std::mutex> lock(m_xStateMutex);
	m_xState = { requestedMode, context.Routing.FallbackReason, context.Producers };

	return context;
}

ShadowVisibilityView ShadowSystem::BuildVisibilityView(const ShadowFrameContext& context,D3D12_GPU_VIRTUAL_ADDRESS emptyBuffer, D3D12_GPU_VIRTUAL_ADDRESS neutralParameters) const {

	ShadowVisibilityView view{};
	auto* atlas = m_pAtlasBackend.get();
	auto* work = m_pNeuralWorkGenerator.get();
	auto* baseline = m_pNeuralBaselinePass.get();

	view.LightMetadata = ShadowRouting::UsesPerLightVisibility(context.Routing) && atlas ? atlas->GetLightsMetasBufferGPULocation() : emptyBuffer;

	view.ShadowParameters = context.Routing.UsesHardShadowAtlas && atlas
		? atlas->GetAtlasConstantBufferGPULocation()
		: context.Routing.UsesRayTraced && atlas
			? atlas->GetPerLightRTConstantBufferGPULocation()
			: neutralParameters;

	view.WorkItems = work && work->GetWorkItemsGPULocation() ? work->GetWorkItemsGPULocation() : emptyBuffer;
	view.WorkHeader = work && work->GetDispatchHeaderGPULocation() ? work->GetDispatchHeaderGPULocation() : emptyBuffer;
	view.PatchRecords = work && work->GetPatchRecordsGPULocation() ? work->GetPatchRecordsGPULocation() : emptyBuffer;
	view.Visibility = baseline && baseline->GetVisibilityGPULocation() ? baseline->GetVisibilityGPULocation() : emptyBuffer;
	view.Validity = baseline && baseline->GetValidityGPULocation() ? baseline->GetValidityGPULocation() : emptyBuffer;
	view.OutputHeader = baseline && baseline->GetOutputHeaderGPULocation() ? baseline->GetOutputHeaderGPULocation() : emptyBuffer;

	if (context.Routing.UsesNeural && m_pNeuralShadowInferencePass) {
		view.NeuralOutputWorkItems = m_pNeuralShadowInferencePass->GetWorkItemsAddress();
		view.NeuralOutputOutputHeader = m_pNeuralShadowInferencePass->GetOutputHeaderAddress();
		view.NeuralOutputVisibility = m_pNeuralShadowInferencePass->GetVisibilityAddress();
		view.NeuralOutputValidity = m_pNeuralShadowInferencePass->GetValidityAddress();
		view.CompactVisibilityEnabled = m_pNeuralShadowInferencePass->GetConsumerMode();
	}
	else {
		view.NeuralOutputWorkItems = emptyBuffer;
		view.NeuralOutputOutputHeader = emptyBuffer;
		view.NeuralOutputVisibility = emptyBuffer;
		view.NeuralOutputValidity = emptyBuffer;
		view.CompactVisibilityEnabled = context.Routing.UsesCompactBaseline ? 1u : 0u;
	}

	return view;
}

ShadowBackendState ShadowSystem::GetState() const {
	std::lock_guard<std::mutex> lock(m_xStateMutex);
	return m_xState;
}

ShadowBackendAvailability ShadowSystem::GetAvailability(bool unlit, bool rtSupported) const {
	return {
		unlit,
		rtSupported,
		m_pAtlasBackend != nullptr,
		m_pAtlasBackend && m_pAtlasBackend->IsPerLightRTReady(),
		m_pAtlasBackend != nullptr,
		m_pAtlasBackend && IsNeuralInferenceReady()
	};
}

void ShadowSystem::EnsureProducerResources(const ShadowProducerRoutingState& producers) {
	if (producers.NeedsNeuralWorkItems && !producers.NeedsNeuralInference) {
		EnsureNeuralWorkGenerator();
		m_pNeuralWorkGenerator->EnsureResourcesAllocated();
	}

	if (!producers.NeedsNeuralInference &&
		(producers.NeedsNeuralFeaturePreparation || producers.NeedsNeuralOutput)) {
		EnsureNeuralBaseline();
		m_pNeuralBaselinePass->EnsureResourcesAllocated();
	}
	if (producers.NeedsNeuralInference) EnsureNeuralRuntimeInitialized();
}

std::shared_ptr<FD3DW::ExecutionHandle> ShadowSystem::ScheduleMetadata(const std::vector<std::shared_ptr<FD3DW::ExecutionHandle>>& dependencies) {
	if (!m_pAtlasBackend) return nullptr;

	auto create = m_pAtlasBackend->OnCreateLightsMeta(dependencies);
	return m_pAtlasBackend->OnUploadLightsMeta(create);
}

std::shared_ptr<FD3DW::ExecutionHandle> ShadowSystem::ScheduleNeuralWork(const std::vector<std::shared_ptr<FD3DW::ExecutionHandle>>& dependencies, bool spatial) {
	if (spatial) return m_pNeuralShadowInferencePass ? m_pNeuralShadowInferencePass->PrepareFrame(dependencies) : nullptr;

	return m_pNeuralWorkGenerator ? m_pNeuralWorkGenerator->GenerateWorkItems(dependencies, true) : nullptr;
}

std::shared_ptr<FD3DW::ExecutionHandle> ShadowSystem::ScheduleHardVisibility( const std::vector<std::shared_ptr<FD3DW::ExecutionHandle>>& dependencies) {
	return m_pAtlasBackend ? m_pAtlasBackend->OnGenerateShadowAtlas(dependencies) : nullptr;
}

std::shared_ptr<FD3DW::ExecutionHandle> ShadowSystem::ScheduleRayTracedVisibility(const std::vector<std::shared_ptr<FD3DW::ExecutionHandle>>& dependencies) {
	return m_pAtlasBackend ? m_pAtlasBackend->OnGeneratePerLightRTVisibility(dependencies) : nullptr;
}

std::shared_ptr<FD3DW::ExecutionHandle> ShadowSystem::ScheduleNeuralOutput( const std::vector<std::shared_ptr<FD3DW::ExecutionHandle>>& dependencies,
	bool useInference) {

	if (useInference && !IsNeuralInferenceReady()) return nullptr;

	if (useInference) return m_pNeuralShadowInferencePass ? m_pNeuralShadowInferencePass->ExecuteFrame(dependencies) : nullptr;

	return m_pNeuralBaselinePass ? m_pNeuralBaselinePass->PrepareAndResolve(dependencies, true) : nullptr;
}

void ShadowSystem::ConfigureInputResources(FD3DW::FResource* normal, ID3D12Device* device) {
	m_pNormalInput = normal;

	if (m_pAtlasBackend) m_pAtlasBackend->SetGBuffersResources(normal, device);

	if (m_pNeuralBaselinePass && m_pAtlasBackend) {
		m_pNeuralBaselinePass->SetInputResources(m_pAtlasBackend->GetShadowAtlas(), normal, device);
	}

	if (m_pNeuralShadowInferencePass) m_pNeuralShadowInferencePass->SetInputResources(normal, device);
}

FD3DW::FResource* ShadowSystem::GetShadowAtlasResource() const {
	return m_pAtlasBackend ? m_pAtlasBackend->GetShadowAtlas() : nullptr;
}

FD3DW::FResource* ShadowSystem::EnsureBlockerDistanceAtlasResource() {
	if (!m_pAtlasBackend) return nullptr;

	m_pAtlasBackend->EnsureBlockerDistanceAtlas();
	return m_pAtlasBackend->GetBlockerDistanceAtlas();
}

FD3DW::FResource* ShadowSystem::GetBlockerValidityAtlasResource() const {
	return m_pAtlasBackend ? m_pAtlasBackend->GetBlockerValidityAtlas() : nullptr;
}

NSRTQualitySettings ShadowSystem::GetRTQualitySettings() const {
	return m_pAtlasBackend ? m_pAtlasBackend->GetNSRTQualitySettings() : NSRTQualitySettings{};
}

void ShadowSystem::SetRTQualitySettings(const NSRTQualitySettings& settings) {
	if (m_pAtlasBackend) m_pAtlasBackend->SetNSRTQualitySettings(settings);
}

HardShadowAtlasFilterSettings ShadowSystem::GetHardFilterSettings() const {
	return m_pAtlasBackend ? m_pAtlasBackend->GetHardShadowAtlasFilterSettings() : HardShadowAtlasFilterSettings{};
}

void ShadowSystem::SetHardFilterSettings(const HardShadowAtlasFilterSettings& settings) {
	if (m_pAtlasBackend) m_pAtlasBackend->SetHardShadowAtlasFilterSettings(settings);
}

ID3D12Device* ShadowSystem::GetDevice() const {
	return m_pOwner->GetDevice();
}

ID3D12Device5* ShadowSystem::GetDXRDevice() const {
	return m_pOwner->GetDXRDevice();
}

int ShadowSystem::GetSceneRenderWidth() const {
	return m_pOwner->GetSceneRenderWidth();
}

int ShadowSystem::GetSceneRenderHeight() const {
	return m_pOwner->GetSceneRenderHeight();
}

DirectX::XMFLOAT3 ShadowSystem::GetCurrentCameraPosition() const {
	return m_pOwner->GetCurrentCameraPosition();
}

float ShadowSystem::GetCameraZFar() const {
	return m_pOwner->GetCameraFrustum().GetZFar();
}

DirectX::XMMATRIX ShadowSystem::GetJitteredViewProjectionMatrix() const {
	return m_pOwner->GetJitteredViewProjectionMatrix();
}

const std::vector<LightComponentData>& ShadowSystem::GetLightComponentsData() const {
	return m_pOwner->GetLightComponentsData();
}

const std::vector<std::uint64_t>& ShadowSystem::GetLightRuntimeIdentities() const {
	return m_pOwner->GetLightRuntimeIdentities();
}

ID3D12Resource* ShadowSystem::GetTLASResource() const {
	return m_pOwner->GetTLAS().pResult.Get();
}

FD3DW::DepthStencilView* ShadowSystem::GetCurrentDepthResource() const {
	return m_pOwner->GetCurrentDSV();
}

D3D12_GPU_VIRTUAL_ADDRESS ShadowSystem::GetLightsBufferAddress() const {
	return m_pOwner->GetLightsBuffer()->GetResource()->GetGPUVirtualAddress();
}

NSRTGeometryCoverageState ShadowSystem::GetRTGeometryCoverage() const {
	return m_pOwner->GetRTGeometryCoverageState();
}

int ShadowSystem::GetLightsCount() const {
	return m_pOwner->GetLightsCount();
}

UINT ShadowSystem::GetShadowMetadataCount() const {
	return m_pAtlasBackend ? m_pAtlasBackend->GetLightsMetaCount() : 0u;
}

void ShadowSystem::PrepareShadowMetadataForCompute(ID3D12GraphicsCommandList* list) const {
	if (m_pAtlasBackend) m_pAtlasBackend->PrepareLightsMetaForCompute(list);
}

void ShadowSystem::PrepareShadowMetadataForPixel(ID3D12GraphicsCommandList* list) const {
	if (m_pAtlasBackend) m_pAtlasBackend->PrepareLightsMetaForPixel(list);
}

D3D12_GPU_VIRTUAL_ADDRESS ShadowSystem::GetClusterBufferAddress() const {
	return m_pOwner->GetClusterStructuredBufferAddress();
}

D3D12_GPU_VIRTUAL_ADDRESS ShadowSystem::GetShadowMetadataAddress() const {
	return m_pAtlasBackend ? m_pAtlasBackend->GetLightsMetasBufferGPULocation() : 0u;
}

FD3DW::FResource* ShadowSystem::GetShadowMetadataResource() const {
	return m_pAtlasBackend ? m_pAtlasBackend->GetLightsMetasBufferResource() : nullptr;
}

float ShadowSystem::GetCameraNear() const {
	return m_pOwner->GetCameraNear();
}

float ShadowSystem::GetCameraFar() const {
	return m_pOwner->GetCameraFar();
}

FD3DW::StructuredBuffer* ShadowSystem::GetLightsBuffer() const {
	return m_pOwner->GetLightsBuffer();
}

D3D12_GPU_VIRTUAL_ADDRESS ShadowSystem::GetNeuralWorkItemsAddress() const {
	return m_pNeuralWorkGenerator ? m_pNeuralWorkGenerator->GetWorkItemsGPULocation() : 0u;
}

D3D12_GPU_VIRTUAL_ADDRESS ShadowSystem::GetNeuralWorkHeaderAddress() const {
	return m_pNeuralWorkGenerator ? m_pNeuralWorkGenerator->GetDispatchHeaderGPULocation() : 0u;
}

FD3DW::FResource* ShadowSystem::GetNeuralWorkItemsResource() const {
	return m_pNeuralWorkGenerator ? m_pNeuralWorkGenerator->GetWorkItemsResource() : nullptr;
}

FD3DW::FResource* ShadowSystem::GetNeuralWorkHeaderResource() const {
	return m_pNeuralWorkGenerator ? m_pNeuralWorkGenerator->GetDispatchHeaderBufferResource() : nullptr;
}
