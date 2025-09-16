#include <MainRenderer/MainRenderer.h>
#include <MainRenderer/PSOManager.h>
#include <MainRenderer/GlobalTextureHeap.h>
#include <D3DFramework/Utilites/Serializer/BinarySerializer.h>
#include <D3DFramework/GraphicUtilites/CommandList.h>

#include <Component/Camera/CameraComponent.h>
#include <Entity/Camera/TBaseCamera.h>

static FLOAT COLOR[4] = { 0.2f,0.2f,0.2f,1.0f };

MainRenderer::MainRenderer() : WinWindow(L"FDW", 1024, 1024, false) {}

void MainRenderer::UserInit()
{
	auto device = GetDevice();
	auto dxrDevice = GetDXRDevice();

	InitMainRendererParts(device);
	if(dxrDevice) InitMainRendererDXRParts(dxrDevice);

	SetVSync(true);

	m_pDSV = CreateDepthStencilView(DXGI_FORMAT_R24G8_TYPELESS, DXGI_FORMAT_D24_UNORM_S8_UINT, DXGI_FORMAT_R24_UNORM_X8_TYPELESS, D3D12_DSV_DIMENSION_TEXTURE2D, 1, 1024, 1024);
	
	m_pDSVPack = CreateDSVPack(1u);
	m_pDSVPack->PushResource(m_pDSV->GetResource(), m_pDSV->GetDSVDesc(), device);

	auto wndSettings = GetMainWNDSettings();

	const auto& gBufferFormats = GetGBufferData().GBuffersFormats;
	auto gbuffersNum = (UINT)gBufferFormats.size();
	m_pGBuffersRTVPack = CreateRTVPack(gbuffersNum);
	m_pGBuffersSRVPack = CreateSRVPack(COUNT_SRV_IN_GBUFFER_HEAP);
	
	m_pGBuffersSRVPack->AddNullResource(SHADOW_FACTOR_LOCATION_IN_HEAP, device);

	for (const auto& format : gBufferFormats) {
		m_pGBuffers.push_back(CreateRenderTarget(format, D3D12_RTV_DIMENSION_TEXTURE2D, 1, wndSettings.Width, wndSettings.Height));
		auto& gbuffer = m_pGBuffers.back();
		
		m_pGBuffersRTVPack->PushResource(gbuffer->GetRTVResource(), gbuffer->GetRTVDesc(), device);
		m_pGBuffersSRVPack->PushResource(device, gbuffer->GetRTVResource(), D3D12_SRV_DIMENSION_TEXTURE2D);
	}

	auto ltcRecipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_DIRECT, [this](ID3D12GraphicsCommandList* list) {
		auto device = GetDevice();
		auto size = GetCBV_SRV_UAVDescriptorSize(device);

		auto LTCMat = FD3DW::FResource::CreateTextureFromPath(LIGHTS_LTC_TEXTURES_PATH_MAT, device, list);
		m_pGBuffersSRVPack->AddResource(LTCMat->GetResource(), D3D12_SRV_DIMENSION_TEXTURE2D, LIGHTS_LTC_MAT_LOCATION_IN_HEAP, device);

		auto LTCAmp = FD3DW::FResource::CreateTextureFromPath(LIGHTS_LTC_TEXTURES_PATH_AMP, device, list);
		m_pGBuffersSRVPack->AddResource(LTCAmp->GetResource(), D3D12_SRV_DIMENSION_TEXTURE2D, LIGHTS_LTC_AMP_LOCATION_IN_HEAP, device);

		m_vLCTResources.push_back(LTCMat);
		m_vLCTResources.push_back(LTCAmp);
	});
	GlobalRenderThreadManager::GetInstance()->Submit(ltcRecipe);
	
	m_pForwardRenderPassRTV = CreateRenderTarget(GetForwardRenderPassFormat(), D3D12_RTV_DIMENSION_TEXTURE2D, 1, wndSettings.Width, wndSettings.Height);
	m_pForwardRenderPassRTVPack = CreateRTVPack(1u);
	m_pForwardRenderPassSRVPack = CreateSRVPack(2u);
	m_pForwardRenderPassRTVPack->PushResource(m_pForwardRenderPassRTV->GetRTVResource(), m_pForwardRenderPassRTV->GetRTVDesc(), device);
	m_pForwardRenderPassSRVPack->PushResource(device, m_pForwardRenderPassRTV->GetRTVResource(), D3D12_SRV_DIMENSION_TEXTURE2D);

	auto rtvRectangleCreation = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_DIRECT, [this](ID3D12GraphicsCommandList* list) {
		m_pScreen = CreateRectangle(list);
		m_pSceneVBV_IBV = std::make_unique<FD3DW::ObjectVertexIndexDataCreator<FD3DW::VertexFrameWork>>();
		m_pSceneVBV_IBV->Create(GetDevice(), list, m_pScreen->GetVertices(), m_pScreen->GetIndices());
		
	});
	GlobalRenderThreadManager::GetInstance()->Submit(rtvRectangleCreation);

	m_xSceneViewPort.MaxDepth = 1.0f;
	m_xSceneViewPort.MinDepth = 0.0f;
	m_xSceneViewPort.Height = (float)wndSettings.Height;
	m_xSceneViewPort.Width = (float)wndSettings.Width;
	m_xSceneViewPort.TopLeftX = 0;
	m_xSceneViewPort.TopLeftY = 0;

	m_xSceneRect.left = 0;
	m_xSceneRect.right = wndSettings.Width;
	m_xSceneRect.top = 0;
	m_xSceneRect.bottom = wndSettings.Height;

	m_pWorld = CreateEmptyWorld();
	InitMainRendererSystems(device);
	if (dxrDevice) InitMainRendererDXRSystems(dxrDevice);

	GlobalRenderThreadManager::GetInstance()->WaitIdle();
	FD3DW::FResource::ReleaseUploadBuffers();

	//CreateTestWorld();

	GlobalRenderThreadManager::GetInstance()->WaitIdle();
	ProcessNotifiesInWorld();
}

void MainRenderer::UserLoop()
{	
	auto sync = m_dInFlight.empty() ? nullptr : m_dInFlight.back();

	auto cameraH = m_pCameraSystem->OnStartTick(sync);

	auto audioH = m_pAudioSystem->OnStartTick(sync);

	auto lightsH = m_pLightSystem->OnStartRenderTick(sync);


	auto meshH = m_pRenderMeshesSystem->OnStartRenderTick(cameraH);

	auto skyboxH = m_pSkyboxRenderSystem->OnStartRenderTick(cameraH);

	auto animationH = m_pSceneAnimationSystem->OnStartRenderTick(sync);

	auto animationGpuSkinningH = m_pSceneAnimationSystem->ProcessGPUSkinning(animationH);

	std::shared_ptr<FD3DW::ExecutionHandle> rtShadowsH = nullptr;
	std::shared_ptr<FD3DW::ExecutionHandle> tlasCallH = nullptr;
	std::shared_ptr<FD3DW::ExecutionHandle> blasCallH = nullptr;

	if (IsRTSupported()) {

		blasCallH = m_pRenderMeshesSystem->OnStartBLASCall({ animationGpuSkinningH , meshH });
		
		tlasCallH = m_pRenderMeshesSystem->OnStartTLASCall({ blasCallH, meshH });

		rtShadowsH = m_pRTShadowSystem->OnStartRenderTick(cameraH);
	}

	auto clusteredH = m_pClusteredLightningSystem->OnStartRenderTick(cameraH);
	auto clusterAssignH = m_pClusteredLightningSystem->AssignLightsToClusters({clusteredH, lightsH, cameraH });

	std::shared_ptr<FD3DW::ExecutionHandle> preDepthH = nullptr;
	if (m_bIsEnabledPreDepth)
	{
		RenderMeshesSystemPreDepthRenderData inPredepthData;
		inPredepthData.DSV = m_pDSV.get();
		inPredepthData.DSV_CPU = m_pDSVPack->GetResult()->GetCPUDescriptorHandle(0);
		inPredepthData.Rect = m_xSceneRect;
		inPredepthData.Viewport = m_xSceneViewPort;

		preDepthH = m_pRenderMeshesSystem->PreDepthRender({meshH, animationGpuSkinningH}, inPredepthData);
	}

	std::shared_ptr<FD3DW::ExecutionHandle> HiZUpdateH = nullptr;
	if (m_pRenderMeshesSystem->GetCullingType() == MeshCullingType::GPU) {
		RenderMeshesSystemHiZUpdateRenderData inHiZData;
		inHiZData.DSV = m_pDSV.get();
		HiZUpdateH = m_pRenderMeshesSystem->UpdateHiZResource({preDepthH, sync}, inHiZData);
	}

	RenderMeshesSystemIndirectRenderData inIndirectData;
	inIndirectData.DSV = m_pDSV.get();
	inIndirectData.DSV_CPU = m_pDSVPack->GetResult()->GetCPUDescriptorHandle(0);
	for (auto& gBufferPtr : m_pGBuffers) inIndirectData.RTV.push_back(gBufferPtr.get());
	inIndirectData.RTV_CPU = m_pGBuffersRTVPack->GetResult()->GetCPUDescriptorHandle(0);
	inIndirectData.Rect = m_xSceneRect;
	inIndirectData.Viewport = m_xSceneViewPort;
	auto indirectRenderH = m_pRenderMeshesSystem->IndirectRender({meshH, HiZUpdateH, preDepthH, cameraH, animationGpuSkinningH }, inIndirectData);


	std::shared_ptr<FD3DW::ExecutionHandle> renderRTShadows = nullptr;
	if (IsRTSupported()) {
		renderRTShadows = m_pRTShadowSystem->OnRenderShadowFactors({ tlasCallH , rtShadowsH, lightsH, clusterAssignH, indirectRenderH } );
	}

	//////////////////////////
	// SHADING PASS
	auto gSecondPassRecipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_DIRECT, [this](ID3D12GraphicsCommandList* list) {
		list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_pForwardRenderPassRTV->StartDraw(list);

		list->RSSetScissorRects(1, &m_xSceneRect);
		list->RSSetViewports(1, &m_xSceneViewPort);

		list->ClearRenderTargetView(m_pForwardRenderPassRTVPack->GetResult()->GetCPUDescriptorHandle(0), COLOR, 0, nullptr);
		list->OMSetRenderTargets(1, &FD3DW::keep(m_pForwardRenderPassRTVPack->GetResult()->GetCPUDescriptorHandle(0)), true, &FD3DW::keep(m_pDSVPack->GetResult()->GetCPUDescriptorHandle(0)));

		PSOManager::GetInstance()->GetPSOObject(PSOType::DefferedSecondPassDefaultConfig)->Bind(list);

		list->IASetVertexBuffers(0, 1, m_pSceneVBV_IBV->GetVertexBufferView());
		list->IASetIndexBuffer(m_pSceneVBV_IBV->GetIndexBufferView());

		ID3D12DescriptorHeap* heaps[] = { m_pGBuffersSRVPack->GetResult()->GetDescriptorPtr() };
		list->SetDescriptorHeaps(_countof(heaps), heaps);

		list->SetGraphicsRootDescriptorTable(DEFFERED_GBUFFERS_POS_IN_ROOT_SIG, m_pGBuffersSRVPack->GetResult()->GetGPUDescriptorHandle(0));

		list->SetGraphicsRootConstantBufferView(LIGHTS_HELPER_BUFFER_POS_IN_ROOT_SIG, m_pLightSystem->GetLightsConstantBufferGPULocation());
		list->SetGraphicsRootShaderResourceView(LIGHTS_BUFFER_POS_IN_ROOT_SIG, m_pLightSystem->GetLightsStructuredBufferGPULocation());
		list->SetGraphicsRootShaderResourceView(LIGHTS_CLUSTERS_BUFFER_POS_IN_ROOT_SIG, m_pClusteredLightningSystem->GetClusteredStructuredBufferGPULocation());
		list->SetGraphicsRootConstantBufferView(LIGHTS_CLUSTERS_DATA_BUFFER_POS_IN_ROOT_SIG, m_pClusteredLightningSystem->GetClusteredConstantBufferGPULocation());

		list->DrawIndexedInstanced(GetIndexSize(m_pScreen.get(), 0), 1, GetIndexStartPos(m_pScreen.get(), 0), GetVertexStartPos(m_pScreen.get(), 0), 0);

		m_pForwardRenderPassRTV->EndDraw(list);
	});

	auto shadingPassH = GlobalRenderThreadManager::GetInstance()->Submit(gSecondPassRecipe, { renderRTShadows, lightsH, clusterAssignH, indirectRenderH });

	SkyboxRenderPassInput inSkyboxRenderData;
	inSkyboxRenderData.RTV = m_pForwardRenderPassRTV.get();
	inSkyboxRenderData.RTV_CPU = m_pForwardRenderPassRTVPack->GetResult()->GetCPUDescriptorHandle(0);
	inSkyboxRenderData.DSV_CPU = m_pDSVPack->GetResult()->GetCPUDescriptorHandle(0);
	inSkyboxRenderData.Rect = m_xSceneRect;
	inSkyboxRenderData.Viewport = m_xSceneViewPort;
	auto skyboxRenderH = m_pSkyboxRenderSystem->RenderSkyboxPass(shadingPassH, inSkyboxRenderData);

	auto bloomPassH = m_pBloomEffectSystem->ProcessBloomPass(skyboxRenderH);

	auto gPostProcessPass = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_DIRECT, [this](ID3D12GraphicsCommandList* list) {
		//////////////////////////
		// POSTPROCESS PASS
		list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		BindMainViewPort(list);
		BindMainRect(list);
		BeginDraw(list);

		{

			list->ClearRenderTargetView(GetCurrBackBufferView(), COLOR, 0, nullptr);
			list->OMSetRenderTargets(1, &FD3DW::keep(GetCurrBackBufferView()), true, nullptr);

			PSOManager::GetInstance()->GetPSOObject(PSOType::PostProcessDefaultConfig)->Bind(list);

			list->IASetVertexBuffers(0, 1, m_pSceneVBV_IBV->GetVertexBufferView());
			list->IASetIndexBuffer(m_pSceneVBV_IBV->GetIndexBufferView());

			ID3D12DescriptorHeap* heaps[] = { m_pForwardRenderPassSRVPack->GetResult()->GetDescriptorPtr() };
			list->SetDescriptorHeaps(_countof(heaps), heaps);
			auto id = m_pBloomEffectSystem->IsEnabledBloom() ? 1 : 0;
			list->SetGraphicsRootDescriptorTable(0,  m_pForwardRenderPassSRVPack->GetResult()->GetGPUDescriptorHandle(id));


			list->DrawIndexedInstanced(GetIndexSize(m_pScreen.get(), 0), 1, GetIndexStartPos(m_pScreen.get(), 0), GetVertexStartPos(m_pScreen.get(), 0), 0);

		}

		/////
		/////////////////////////////


		////////////////////////////
		////			UI PASS

		{
			m_pUIComponent->RenderImGui(list);
		}

		/////
		/////////////////////////////

		{
			EndDraw(list);
		}
	});

	auto postProcessPass = GlobalRenderThreadManager::GetInstance()->Submit(gPostProcessPass, { shadingPassH, skyboxRenderH, bloomPassH });

	auto presentH = GlobalRenderThreadManager::GetInstance()->SubmitLambda([this]() { 
		PresentSwapchain(); 
		++m_uFrameIndex; 	
	}, { postProcessPass }, true);

	m_dInFlight.push_back(presentH);

	m_pUIComponent->ProcessAfterRenderUICalls();
	ProcessNotifiesInWorld();
	
	while (m_dInFlight.size() >= m_uMaxFramesInFlight) {
		auto& front = m_dInFlight.front();
		if (front && !front->IsDone()) {
			front->WaitForExecute();
		}
		m_dInFlight.pop_front();
	}

	GlobalRenderThreadManager::GetInstance()->GarbageCollectAll();

}

void MainRenderer::UserClose()
{
	GlobalRenderThreadManager::GetInstance()->Shutdown();
	
	for (auto& system : m_vSystems) {
		DestroyComponent(system);
	}
	DestroyComponent(m_pUIComponent);

	PSOManager::FreeInstance();
}

FD3DW::BaseCommandQueue* MainRenderer::UserSwapchainCommandQueue(ID3D12Device* device)
{
	GlobalRenderThreadManager::GetInstance()->Init(device);
	return GlobalRenderThreadManager::GetInstance()->GetQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
}

UINT MainRenderer::GetFrameIndex()
{
	return m_uFrameIndex;
}

bool MainRenderer::IsEnabledPreDepth() {
	return m_bIsEnabledPreDepth;
}

void MainRenderer::EnablePreDepth(bool in) {
	m_bIsEnabledPreDepth = in;
}

bool MainRenderer::IsEnabledBloom() {
	return m_pBloomEffectSystem->IsEnabledBloom();
}

void MainRenderer::EnableBloom(bool b) {
	m_pBloomEffectSystem->EnableBloom(b);
}

FLOAT* MainRenderer::GetClearColor(){
	return COLOR;
}

World* MainRenderer::GetWorld() {
	return m_pWorld.get();
}

dx::XMMATRIX MainRenderer::GetCurrentProjectionMatrix() const {
	return m_pCameraSystem->GetProjectionMatrix();
}

dx::XMMATRIX MainRenderer::GetCurrentViewMatrix() const {
	return m_pCameraSystem->GetViewMatrix();
}

dx::XMFLOAT3 MainRenderer::GetCurrentCameraPosition() const {
	return m_pCameraSystem->GetCameraPosition();
}

CameraFrustum MainRenderer::GetCameraFrustum() {
	return m_pCameraSystem->GetCameraFrustum();
}

void MainRenderer::SetMeshCullingType(MeshCullingType in) {
	m_pRenderMeshesSystem->SetCullingType(in);
}

MeshCullingType MainRenderer::GetMeshCullingType() {
	return m_pRenderMeshesSystem->GetCullingType();
}

FD3DW::AccelerationStructureBuffers MainRenderer::GetTLAS()
{
	return m_pRenderMeshesSystem->GetTLAS();
}

D3D12_GPU_VIRTUAL_ADDRESS MainRenderer::GetLightBufferConstantBufferAddress() {
	return m_pLightSystem->GetLightsConstantBufferGPULocation();
}

D3D12_GPU_VIRTUAL_ADDRESS MainRenderer::GetLightsStructuredBufferAddress() {
	return m_pLightSystem->GetLightsStructuredBufferGPULocation();
}

D3D12_GPU_VIRTUAL_ADDRESS MainRenderer::GetClusterConstantBufferAddress() {
	return m_pClusteredLightningSystem->GetClusteredConstantBufferGPULocation();
}

D3D12_GPU_VIRTUAL_ADDRESS MainRenderer::GetClusterStructuredBufferAddress() {
	return m_pClusteredLightningSystem->GetClusteredStructuredBufferGPULocation();
}
FD3DW::StructuredBuffer* MainRenderer::GetLightsBuffer() {
	return m_pLightSystem->GetLightsBuffer();
}

int MainRenderer::GetLightsCount() {
	return m_pLightSystem->GetLightsCount();
}

bool MainRenderer::IsShadowEnabled() {
	return IsRTSupported();
}

void MainRenderer::SetRTShadowConfig(RTShadowSystemConfig config) {
	if(m_pRTShadowSystem) m_pRTShadowSystem->SetConfig(config);
}

RTShadowSystemConfig MainRenderer::GetRTShadowConfig() {
	return m_pRTShadowSystem ? m_pRTShadowSystem->GetConfig() : RTShadowSystemConfig();
}

std::shared_ptr<World> MainRenderer::CreateEmptyWorld() {
	auto world = std::make_shared<World>();
	world->SetMainRenderer(this);
	return world;
}

void MainRenderer::LoadWorld(std::string pathTo) {
	GlobalRenderThreadManager::GetInstance()->WaitIdle();

	BinarySerializer ser;
	ser.LoadFromFile(pathTo);

	ser.DeserializeToObjects(m_pWorld);
	m_pWorld->SetMainRenderer(this);
}

void MainRenderer::SaveActiveWorld(std::string pathTo) {
	GlobalRenderThreadManager::GetInstance()->WaitIdle();

	BinarySerializer ser;
	ser.LoadFromObjects(m_pWorld);
	ser.SaveToFile(pathTo);
}

BloomSystemCompositeData MainRenderer::GetCompositeData() { return m_pBloomEffectSystem->GetCompositeData(); }
void MainRenderer::SetCompositeData(BloomSystemCompositeData data) { m_pBloomEffectSystem->SetCompositeData(data); }

BloomSystemBrightPassData MainRenderer::GetBrightPassData() { return m_pBloomEffectSystem->GetBrightPassData(); }
void MainRenderer::SetBrightPassData(BloomSystemBrightPassData data) { m_pBloomEffectSystem->SetBrightPassData(data); }


BloomBlurType MainRenderer::GetBloomBlurType() {
	return m_pBloomEffectSystem->GetBloomBlurType();
}

void MainRenderer::SetBloomBlurType(BloomBlurType blurType) {
	m_pBloomEffectSystem->SetBloomBlurType(blurType);
}

void MainRenderer::CreateTestWorld() {
	m_pWorld->CreateDefaultCamera();
	
	auto light = m_pWorld->CreatePointLight();
	auto pos = light->GetLightPosition();
	pos.y = 444.0f;
	light->SetLightPosition(pos);
	light->SetLightAttenuationRadius(1111.0f);
	light->SetLightSourceRadius(13.0f);

	m_pWorld->CreateScene("Content/SampleModels/sponza/scene.gltf");
	m_pWorld->CreateSimplePlane();
	m_pWorld->CreateSkybox("Content/Skybox/Moon.dds");
}

void MainRenderer::ProcessNotifiesInWorld() {
	static std::vector<NRenderSystemNotifyType> s_sNotifiesToWaitRenderTick = {
		NRenderSystemNotifyType::Light,
		NRenderSystemNotifyType::SkyboxActivationDeactivation,
		NRenderSystemNotifyType::MeshActivationDeactivation,
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

void MainRenderer::ProcessNotifies(std::vector<NRenderSystemNotifyType> notifies) {
	for (auto& sys : m_vSystems) {
		for (auto notify : notifies) {
			sys->ProcessNotify(notify);
		}
	}
}

void MainRenderer::InitMainRendererParts(ID3D12Device* device) {
	InitializeDescriptorSizes(device, Get_RTV_DescriptorSize(), Get_DSV_DescriptorSize(), Get_CBV_SRV_UAV_DescriptorSize());
	PSOManager::GetInstance()->InitPSOjects(device);
	CreateEmptyStructuredBuffer(device);
	GlobalTextureHeap::GetInstance()->Init(device, GLOBAL_TEXTURE_HEAP_PRECACHE_SIZE,GLOBAL_TEXTURE_HEAP_NODE_MASK);

	m_pUIComponent = CreateUniqueComponent<MainRenderer_UIComponent>();
}

void MainRenderer::InitMainRendererDXRParts(ID3D12Device5* device)
{
	PSOManager::GetInstance()->InitPSOjectsDevice5(device);
}

void MainRenderer::InitMainRendererSystems(ID3D12Device* device) {
	m_pAudioSystem = CreateSystem<AudioSystem>();
	m_pCameraSystem = CreateSystem<CameraSystem>();
	m_pLightSystem = CreateSystem<LightSystem>();;
	m_pClusteredLightningSystem = CreateSystem<ClusteredLightningSystem>();
	m_pSceneAnimationSystem = CreateSystem<SceneAnimationSystem>();
	m_pRenderMeshesSystem = CreateSystem<RenderMeshesSystem>();
	m_pSkyboxRenderSystem = CreateSystem<SkyboxRenderSystem>();

	m_pBloomEffectSystem = CreateSystem<BloomEffectSystem>();
	m_pBloomEffectSystem->SetShadingOutputResourceResultAndRect(m_pForwardRenderPassRTV->GetTexture());
	m_pForwardRenderPassSRVPack->AddResource(m_pBloomEffectSystem->GetResultResource(), D3D12_SRV_DIMENSION_TEXTURE2D, 1, device);
}

void MainRenderer::InitMainRendererDXRSystems(ID3D12Device5* device) {
	m_pRTShadowSystem = CreateSystem<RTShadowSystem>();
	m_pRTShadowSystem->SetGBuffersResources(m_pGBuffers[0]->GetTexture(), m_pGBuffers[1]->GetTexture(), device);
	m_pGBuffersSRVPack->AddResource(m_pRTShadowSystem->GetResultResource()->GetResource(), D3D12_SRV_DIMENSION_TEXTURE2D, SHADOW_FACTOR_LOCATION_IN_HEAP, device);
}
