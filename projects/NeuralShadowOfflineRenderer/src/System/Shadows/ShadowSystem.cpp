#include "pch.h"

#include <System/Shadows/ShadowSystem.h>

#include <Component/Shadow/ShadowComponent.h>
#include <Component/Light/LightComponent.h>
#include <Entity/Core/ComponentHolder.h>
#include <World/World.h>
#include <Renderer/OfflineRenderer.h>

void ShadowSystem::AfterConstruction() {
	RefreshUserSettings();
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
	}
}

void ShadowSystem::EnsureNeuralBaseline() {
	if (m_pNeuralBaselinePass) return;
	
	m_pNeuralBaselinePass = std::make_unique<NeuralShadowBaselinePass>(*this);
	
	if (m_pAtlasBackend && m_pNormalInput) {
		m_pNeuralBaselinePass->SetInputResources(m_pAtlasBackend->GetShadowAtlas(), m_pNormalInput, GetDevice());
	}
}

NeuralShadowWorkGenerator* ShadowSystem::GetNeuralWorkGenerator() const noexcept {
	return m_pNeuralWorkGenerator.get();
}

NeuralShadowBaselinePass* ShadowSystem::GetNeuralBaselinePass() const noexcept {
	return m_pNeuralBaselinePass.get();
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

void ShadowSystem::EnsureCaptureResources() {
	EnsureNeuralWorkGenerator();
	m_pNeuralWorkGenerator->EnsureResourcesAllocated();
	
	EnsureNeuralBaseline();
	m_pNeuralBaselinePass->EnsureResourcesAllocated();
}

std::shared_ptr<FD3DW::ExecutionHandle> ShadowSystem::ScheduleMetadata(const std::vector<std::shared_ptr<FD3DW::ExecutionHandle>>& dependencies) {
	if (!m_pAtlasBackend) return nullptr;
	
	auto create = m_pAtlasBackend->OnCreateLightsMeta(dependencies);
	
	return m_pAtlasBackend->OnUploadLightsMeta(create);
}

std::shared_ptr<FD3DW::ExecutionHandle> ShadowSystem::ScheduleNeuralWork(const std::vector<std::shared_ptr<FD3DW::ExecutionHandle>>& dependencies,std::uint64_t generationOverride) {
	return m_pNeuralWorkGenerator? m_pNeuralWorkGenerator->GenerateWorkItems(dependencies, generationOverride) : nullptr;
}

std::shared_ptr<FD3DW::ExecutionHandle> ShadowSystem::ScheduleHardVisibility(const std::vector<std::shared_ptr<FD3DW::ExecutionHandle>>& dependencies) {
	return m_pAtlasBackend ? m_pAtlasBackend->OnGenerateShadowAtlas(dependencies) : nullptr;
}

std::shared_ptr<FD3DW::ExecutionHandle> ShadowSystem::ScheduleNeuralBaseline(const std::vector<std::shared_ptr<FD3DW::ExecutionHandle>>& dependencies) {
	return m_pNeuralBaselinePass ? m_pNeuralBaselinePass->PrepareAndResolve(dependencies) : nullptr;
}

void ShadowSystem::ConfigureInputResources(FD3DW::FResource* normal, ID3D12Device* device) {
	
	m_pNormalInput = normal;
	
	if (m_pAtlasBackend) m_pAtlasBackend->SetGBuffersResources(normal, device);
	
	if (m_pNeuralBaselinePass && m_pAtlasBackend) {
		m_pNeuralBaselinePass->SetInputResources(m_pAtlasBackend->GetShadowAtlas(), normal, device);
	}
}

FD3DW::FResource* ShadowSystem::EnsureBlockerDistanceAtlasResource() {
	if (!m_pAtlasBackend) return nullptr;
	
	m_pAtlasBackend->EnsureBlockerDistanceAtlas();
	return m_pAtlasBackend->GetBlockerDistanceAtlas();
}

FD3DW::FResource* ShadowSystem::GetBlockerValidityAtlasResource() const {
	return m_pAtlasBackend ? m_pAtlasBackend->GetBlockerValidityAtlas() : nullptr;
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

DirectX::XMMATRIX ShadowSystem::GetViewProjectionMatrix() const {
	return m_pOwner->GetViewProjectionMatrix();
}

const std::vector<LightComponentData>& ShadowSystem::GetLightComponentsData() const {
	return m_pOwner->GetLightComponentsData();
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
