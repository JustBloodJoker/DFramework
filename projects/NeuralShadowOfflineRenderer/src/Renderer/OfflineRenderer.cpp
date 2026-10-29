#include "pch.h"

#include <Renderer/OfflineRenderer.h>
#include <Capture/NeuralShadowCaptureController.h>
#include <Renderer/PSOManager.h>
#include <Renderer/GlobalTextureHeap.h>
#include <D3DFramework/Utilites/Serializer/BinarySerializer.h>
#include <D3DFramework/GraphicUtilites/CommandList.h>

#include <Component/Camera/CameraComponent.h>
#include <Component/RenderObject/MeshComponent.h>
#include <Entity/Camera/TBaseCamera.h>

OfflineRenderer::OfflineRenderer(NeuralShadowCaptureController& captureController) : WinWindow(NSOfflineConfig::RendererLimits::WindowTitle,static_cast<int>(captureController.GetOptions().Config.OutputWidth),static_cast<int>(captureController.GetOptions().Config.OutputHeight),false, false), m_xCaptureController(captureController) {}

void OfflineRenderer::UserInit() {
	auto device = GetDevice();
	auto dxrDevice = GetDXRDevice();
	
	auto captureInitProgress = [this](const char* message) {
		if (m_xCaptureController.IsConfigured()) {
			std::cerr << "[neural-shadow-capture] init: " << message << '\n';
		}
	};

	InitOfflineRendererParts(device);
	
	if(dxrDevice) InitOfflineRendererDXRParts(dxrDevice);
	
	UpdateSceneViewportLayout();
	
	auto sceneWidth = (UINT)std::max(1, GetSceneRenderWidth());
	auto sceneHeight = (UINT)std::max(1, GetSceneRenderHeight());

	m_pDSVPack = CreateDSVPack(2u);
	for (int i = 0; i < 2; ++i) {
		m_pDSV[i] = CreateDepthStencilView(DXGI_FORMAT_R24G8_TYPELESS, DXGI_FORMAT_D24_UNORM_S8_UINT, DXGI_FORMAT_R24_UNORM_X8_TYPELESS, D3D12_DSV_DIMENSION_TEXTURE2D, 1, sceneWidth, sceneHeight);
		m_pDSVPack->PushResource(m_pDSV[i]->GetResource(), m_pDSV[i]->GetDSVDesc(), device);
	}

	const auto& gBufferFormats = GetGBufferData().GBuffersFormats;
	auto gbuffersNum = (UINT)gBufferFormats.size();
	m_pGBuffersRTVPack = CreateRTVPack(gbuffersNum);

	for (const auto& format : gBufferFormats) {
		m_pGBuffers.push_back(CreateRenderTarget(format, D3D12_RTV_DIMENSION_TEXTURE2D, 1, sceneWidth, sceneHeight));
		auto& gbuffer = m_pGBuffers.back();

		m_pGBuffersRTVPack->PushResource(gbuffer->GetRTVResource(), gbuffer->GetRTVDesc(), device);
	}

	captureInitProgress("G-buffer resources ready");

	m_pWorld = CreateEmptyWorld();
	InitOfflineRendererSystems();
	
	if (dxrDevice) InitOfflineRendererDXRSystems(dxrDevice);

	GlobalRenderThreadManager::GetInstance()->WaitIdle();
	FD3DW::FResource::ReleaseUploadBuffers();

	GlobalRenderThreadManager::GetInstance()->WaitIdle();
	ProcessNotifiesInWorld();
	
	if (!m_xCaptureController.Initialize(*this)) {
		m_xCaptureController.Finish(*this, false, m_xCaptureController.m_sCaptureLastError);
	}
}

void OfflineRenderer::UserLoop()
{
	auto sceneRect = m_xSceneRect;
	auto sceneViewPort = m_xSceneViewPort;
	m_pShadowSystem->EnsureCaptureResources();

	auto cameraH = m_pCameraSystem->OnStartTick(nullptr);

	auto lightsH = m_pLightSystem->OnStartRenderTick(nullptr);

	auto uploadMetaH = m_pShadowSystem->ScheduleMetadata({ lightsH, cameraH });

	auto meshH = m_pRenderMeshesSystem->OnStartRenderTick(cameraH);

	auto tlasCallH = m_pRenderMeshesSystem->OnStartTLASCall({ meshH });

	auto clusteredH = m_pClusteredLightningSystem->OnStartRenderTick({ cameraH, lightsH });
	auto clusterAssignH = m_pClusteredLightningSystem->AssignLightsToClusters({ clusteredH, lightsH, cameraH });
	const std::uint64_t captureGeneration = m_xCaptureController.GetGenerationOverride();
	auto neuralShadowWorkH = m_pShadowSystem->ScheduleNeuralWork({ clusterAssignH, uploadMetaH, lightsH, cameraH },captureGeneration);

	RenderMeshesSystemIndirectRenderData inIndirectData;
	inIndirectData.DSV = GetCurrentDSV();
	inIndirectData.DSV_CPU = GetCurrentDSV_CPUAddr();
	
	for (auto& gBufferPtr : m_pGBuffers) inIndirectData.RTV.push_back(gBufferPtr.get());
	
	inIndirectData.RTV_CPU = m_pGBuffersRTVPack->GetResult()->GetCPUDescriptorHandle(0);
	inIndirectData.Rect = sceneRect;
	inIndirectData.Viewport = sceneViewPort;
	
	auto indirectRenderH = m_pRenderMeshesSystem->IndirectRender({meshH, cameraH}, inIndirectData);

	auto atlasRtShadowsH = m_pShadowSystem->ScheduleHardVisibility({ clusterAssignH, neuralShadowWorkH, tlasCallH, lightsH, uploadMetaH, indirectRenderH });

	auto neuralShadowBaselineH = m_pShadowSystem->ScheduleNeuralBaseline({ neuralShadowWorkH, atlasRtShadowsH, indirectRenderH, lightsH, uploadMetaH });

	std::shared_ptr<FD3DW::ExecutionHandle> neuralShadowCaptureH = nullptr;
	const bool isNeuralShadowCaptureWarmup = m_xCaptureController.IsWarmupFrame();
	if (!m_xCaptureController.IsFinished() && !isNeuralShadowCaptureWarmup) {
		neuralShadowCaptureH = m_xCaptureController.Schedule(*this, {
			neuralShadowBaselineH, neuralShadowWorkH, tlasCallH, indirectRenderH, lightsH, uploadMetaH
		});
	}

	auto captureFrameH = GlobalRenderThreadManager::GetInstance()->SubmitLambda([this]() {m_uCurrentDSVIndex = (m_uCurrentDSVIndex + 1) % 2;}, { neuralShadowCaptureH, neuralShadowBaselineH, atlasRtShadowsH,indirectRenderH }, true);
	
	captureFrameH->WaitForExecute();
	
	ProcessNotifiesInWorld();
	GlobalRenderThreadManager::GetInstance()->GarbageCollectAll();

	if (isNeuralShadowCaptureWarmup) {
		--m_xCaptureController.m_uCaptureWarmupFramesRemaining;
	}

	if (neuralShadowCaptureH && !m_xCaptureController.Finalize(*this, neuralShadowCaptureH)) {
		m_xCaptureController.Finish(*this, false, m_xCaptureController.m_sCaptureLastError);
	}
}

void OfflineRenderer::UserClose()
{
	GlobalRenderThreadManager::GetInstance()->Shutdown();

	m_vSystems.clear();

	PSOManager::FreeInstance();
}

FD3DW::BaseCommandQueue* OfflineRenderer::UserSwapchainCommandQueue(ID3D12Device* device)
{
	GlobalRenderThreadManager::GetInstance()->Init(device);
	return GlobalRenderThreadManager::GetInstance()->GetQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
}

World* OfflineRenderer::GetWorld() {
	return m_pWorld.get();
}

int OfflineRenderer::GetSceneRenderWidth() const {
	auto width = int(m_xSceneRect.right - m_xSceneRect.left);
	return std::max(0, width);
}

int OfflineRenderer::GetSceneRenderHeight() const {
	auto height = int(m_xSceneRect.bottom - m_xSceneRect.top);
	return std::max(0, height);
}

FD3DW::DepthStencilView* OfflineRenderer::GetCurrentDSV() {
	return m_pDSV[m_uCurrentDSVIndex].get();
}

D3D12_CPU_DESCRIPTOR_HANDLE OfflineRenderer::GetCurrentDSV_CPUAddr() {
	return m_pDSVPack->GetResult()->GetCPUDescriptorHandle(m_uCurrentDSVIndex);
}

dx::XMMATRIX OfflineRenderer::GetCurrentProjectionMatrix() const {
	return m_pCameraSystem->GetProjectionMatrix();
}

dx::XMMATRIX OfflineRenderer::GetCurrentViewMatrix() const {
	return m_pCameraSystem->GetViewMatrix();
}

dx::XMMATRIX OfflineRenderer::GetViewProjectionMatrix() const {
	return m_pCameraSystem->GetViewProjectionMatrix();
}

dx::XMFLOAT3 OfflineRenderer::GetCurrentCameraPosition() const {
	return m_pCameraSystem->GetCameraPosition();
}

CameraFrustum OfflineRenderer::GetCameraFrustum() {
	return m_pCameraSystem->GetCameraFrustum();
}

float OfflineRenderer::GetCameraNear() const {
	return m_pCameraSystem->GetZNear();
}

float OfflineRenderer::GetCameraFar() const {
	return m_pCameraSystem->GetZFar();
}

FD3DW::AccelerationStructureBuffers OfflineRenderer::GetTLAS()
{
	return m_pRenderMeshesSystem->GetTLAS();
}

NSRTGeometryCoverageState OfflineRenderer::GetRTGeometryCoverageState() const {
	return m_pRenderMeshesSystem ? m_pRenderMeshesSystem->GetRTGeometryCoverageState() : NSRTGeometryCoverageState{};
}

NSRTGeometryAccounting OfflineRenderer::GetRTGeometryAccounting() const {
	return m_pRenderMeshesSystem ? m_pRenderMeshesSystem->GetRTGeometryAccounting() : NSRTGeometryAccounting{};
}

FD3DW::FResource* OfflineRenderer::GetRTGeometryInstancesResource() const {
	return m_pRenderMeshesSystem ? m_pRenderMeshesSystem->GetRTGeometryInstancesResource() : nullptr;
}

ID3D12Resource* OfflineRenderer::GetRTAlphaIndexBuffer() const {
	return m_pRenderMeshesSystem ? m_pRenderMeshesSystem->GetRTAlphaIndexBuffer() : nullptr;
}

ID3D12Resource* OfflineRenderer::GetRTAlphaVertexBuffer() const {
	return m_pRenderMeshesSystem ? m_pRenderMeshesSystem->GetRTAlphaVertexBuffer() : nullptr;
}

D3D12_GPU_VIRTUAL_ADDRESS OfflineRenderer::GetClusterStructuredBufferAddress() {
	return m_pClusteredLightningSystem->GetClusteredStructuredBufferGPULocation();
}

FD3DW::StructuredBuffer* OfflineRenderer::GetLightsBuffer() {
	return m_pLightSystem->GetLightsBuffer();
}

const std::vector<LightComponentData>& OfflineRenderer::GetLightComponentsData() const {
	return m_pLightSystem->GetLightComponentsData();
}

int OfflineRenderer::GetLightsCount() {
	return m_pLightSystem->GetLightsCount();
}

UINT OfflineRenderer::GetShadowMetadataCount() const {
	return m_pShadowSystem ? m_pShadowSystem->GetShadowMetadataCount() : 0u;
}

void OfflineRenderer::SetHardShadowAtlasFilterSettings(const HardShadowAtlasFilterSettings& settings) {
	if (m_pShadowSystem) m_pShadowSystem->SetHardFilterSettings(settings);
}

std::shared_ptr<World> OfflineRenderer::CreateEmptyWorld() {
	auto world = std::make_shared<World>();
	world->SetOfflineRenderer(this);
	return world;
}

void OfflineRenderer::LoadWorld(std::string pathTo) {
	GlobalRenderThreadManager::GetInstance()->WaitIdle();

	BinarySerializer ser;
	ser.LoadFromFile(pathTo);

	ser.DeserializeToObjects(m_pWorld);

	m_pWorld->SetOfflineRenderer(this);
}

void OfflineRenderer::ProcessNotifiesInWorld() {
	static std::vector<NRenderSystemNotifyType> s_sNotifiesToWaitRenderTick = {
		NRenderSystemNotifyType::Light,
		NRenderSystemNotifyType::MeshActivationDeactivation
	};

	auto checkNotifies = m_pWorld->GetRenderSystemNotifies();
	m_pWorld->ClearNotifies();
	bool hasMatch = std::any_of(checkNotifies.begin(), checkNotifies.end(),
		[](NRenderSystemNotifyType n) {
			return std::find(s_sNotifiesToWaitRenderTick.begin(), s_sNotifiesToWaitRenderTick.end(), n) != s_sNotifiesToWaitRenderTick.end();
		}
	);

	if (checkNotifies.empty()) return;

	if (hasMatch) GlobalRenderThreadManager::GetInstance()->WaitIdle();

	ProcessNotifies(checkNotifies);

	if (hasMatch) GlobalRenderThreadManager::GetInstance()->WaitIdle();
}

void OfflineRenderer::ProcessNotifies(std::vector<NRenderSystemNotifyType> notifies) {
	for (auto& sys : m_vSystems) {
		for (auto notify : notifies) {
			sys->ProcessNotify(notify);
		}
	}
}

void OfflineRenderer::UpdateSceneViewportLayout() {
	const auto& windowSettings = WNDSettings();
	const auto width = std::max(1, static_cast<int>(windowSettings.Width));
	const auto height = std::max(1, static_cast<int>(windowSettings.Height));

	m_xSceneRect = {0, 0, width, height};
	m_xSceneViewPort = {0.0f,0.0f,static_cast<float>(width),static_cast<float>(height),0.0f,1.0f};
}

void OfflineRenderer::InitOfflineRendererParts(ID3D12Device* device) {
	InitializeDescriptorSizes(device, Get_RTV_DescriptorSize(), Get_CBV_SRV_UAV_DescriptorSize());
	
	PSOManager::GetInstance()->InitializePSOs(device);
	
	CreateEmptyStructuredBuffer(device);
	
	GlobalTextureHeap::GetInstance()->Init(device, NSOfflineConfig::RendererLimits::TextureHeapPrecacheSize,NSOfflineConfig::RendererLimits::TextureHeapNodeMask);
}

void OfflineRenderer::InitOfflineRendererDXRParts(ID3D12Device5* device) {
	PSOManager::GetInstance()->InitializeRayTracingPSOs(device);
}

void OfflineRenderer::InitOfflineRendererSystems() {
	m_pCameraSystem = CreateSystem<CameraSystem>();
	m_pLightSystem = CreateSystem<LightSystem>();
	m_pClusteredLightningSystem = CreateSystem<ClusteredLightningSystem>();
	m_pShadowSystem = CreateSystem<ShadowSystem>();
	m_pRenderMeshesSystem = CreateSystem<RenderMeshesSystem>();
}

void OfflineRenderer::InitOfflineRendererDXRSystems(ID3D12Device5* device) {
	m_pShadowSystem->InitializeBackend();
	m_pShadowSystem->ConfigureInputResources(m_pGBuffers[GBUFFER_NORMAL_LOCATION_IN_HEAP]->GetTexture(), device);
}
