#include <MainRenderer/MainRenderer.h>
#include <MainRenderer/PSOManager.h>
#include <MainRenderer/GlobalTextureHeap.h>
#include <D3DFramework/Utilites/Serializer/BinarySerializer.h>
#include <D3DFramework/GraphicUtilites/CommandList.h>

#include <Component/Camera/CameraComponent.h>
#include <Component/RenderObject/MeshComponent.h>
#include <Entity/Camera/TBaseCamera.h>

static FLOAT COLOR[4] = { 0.2f,0.2f,0.2f,3.0f };

MainRenderer::MainRenderer() : WinWindow(WINDOW_TITLE, WINDOW_START_WIDTH, WINDOW_START_HEIGHT, false) {}

void MainRenderer::UserInit()
{
	auto device = GetDevice();
	auto dxrDevice = GetDXRDevice();

	InitMainRendererParts(device);
	if(dxrDevice) InitMainRendererDXRParts(dxrDevice);

	SetVSync(true);
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
	m_pGBuffersSRVPack = CreateSRVPack(COUNT_SRV_IN_GBUFFER_HEAP);
	
	m_pGBuffersSRVPack->AddNullResource(SHADOW_FACTOR_LOCATION_IN_HEAP, device);

	for (const auto& format : gBufferFormats) {
		m_pGBuffers.push_back(CreateRenderTarget(format, D3D12_RTV_DIMENSION_TEXTURE2D, 1, sceneWidth, sceneHeight));
		auto& gbuffer = m_pGBuffers.back();
		
		m_pGBuffersRTVPack->PushResource(gbuffer->GetRTVResource(), gbuffer->GetRTVDesc(), device);
		m_pGBuffersSRVPack->PushResource(device, gbuffer->GetRTVResource(), D3D12_SRV_DIMENSION_TEXTURE2D);
	}

	m_pGBuffersSRVPack->AddResource(GetCurrentDSV(), DEPTH_BUFFER_LOCATION_IN_HEAP, device);

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
	
	m_pForwardRenderPassRTV = CreateRenderTarget(GetForwardRenderPassFormat(), D3D12_RTV_DIMENSION_TEXTURE2D, 1, sceneWidth, sceneHeight);
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

	m_pWorld = CreateEmptyWorld();
	InitMainRendererSystems(device);
	if (dxrDevice) InitMainRendererDXRSystems(dxrDevice);

	GlobalRenderThreadManager::GetInstance()->WaitIdle();
	FD3DW::FResource::ReleaseUploadBuffers();

	GlobalRenderThreadManager::GetInstance()->WaitIdle();
	ProcessNotifiesInWorld();
}

void MainRenderer::UserLoop()
{	
	if (auto timer = GetTimer(); m_pWorld && timer) {
		m_pWorld->UpdateScripts(timer->GetDeltaTime());
		ProcessNotifiesInWorld();
	}

	ValidateSelection();

	auto sceneRect = m_xSceneRect;
	auto sceneViewPort = m_xSceneViewPort;
	auto sceneOutputViewPort = m_xSceneOutputViewPort;
	auto mainViewPort = m_xMainViewPort;
	auto mainRect = m_xMainRect;

	auto sync = m_dInFlight.empty() ? nullptr : m_dInFlight.back();

	auto cameraH = m_pCameraSystem->OnStartTick(sync);

	auto audioH = m_pAudioSystem->OnStartTick(sync);

	auto lightsH = m_pLightSystem->OnStartRenderTick(sync);

	std::shared_ptr<FD3DW::ExecutionHandle> uploadMetaH = nullptr;
	if (IsRTSupported()) {
		auto createMetaH = m_pAtlasRTShadowSystem->OnCreateLightsMeta({ lightsH, cameraH });
		uploadMetaH = m_pAtlasRTShadowSystem->OnUploadLightsMeta(createMetaH);
	}

	auto meshH = m_pRenderMeshesSystem->OnStartRenderTick(cameraH);

	auto skyboxH = m_pSkyboxRenderSystem->OnStartRenderTick(cameraH);

	auto animationH = m_pSceneAnimationSystem->OnStartRenderTick(sync);

	auto animationGpuSkinningH = m_pSceneAnimationSystem->ProcessGPUSkinning(animationH);

	std::shared_ptr<FD3DW::ExecutionHandle> tlasCallH = nullptr;
	std::shared_ptr<FD3DW::ExecutionHandle> blasCallH = nullptr;

	if (IsRTSupported()) {
		blasCallH = m_pRenderMeshesSystem->OnStartBLASCall({ animationGpuSkinningH , meshH });
		tlasCallH = m_pRenderMeshesSystem->OnStartTLASCall({ blasCallH, meshH });
	}

	auto clusteredH = m_pClusteredLightningSystem->OnStartRenderTick(cameraH);
	auto clusterAssignH = m_pClusteredLightningSystem->AssignLightsToClusters({clusteredH, lightsH, cameraH });

	std::shared_ptr<FD3DW::ExecutionHandle> preDepthH = nullptr;
	if (m_bIsEnabledPreDepth)
	{
		RenderMeshesSystemPreDepthRenderData inPredepthData;
		inPredepthData.DSV = GetCurrentDSV();
		inPredepthData.DSV_CPU = GetCurrentDSV_CPUAddr();
		inPredepthData.Rect = sceneRect;
		inPredepthData.Viewport = sceneViewPort;

		preDepthH = m_pRenderMeshesSystem->PreDepthRender({meshH, animationGpuSkinningH}, inPredepthData);
	}

	std::shared_ptr<FD3DW::ExecutionHandle> HiZUpdateH = nullptr;
	if (m_pRenderMeshesSystem->GetCullingType() == MeshCullingType::GPU) {
		RenderMeshesSystemHiZUpdateRenderData inHiZData;
		inHiZData.DSV = GetCurrentDSV();
		HiZUpdateH = m_pRenderMeshesSystem->UpdateHiZResource({preDepthH, sync}, inHiZData);
	}

	RenderMeshesSystemIndirectRenderData inIndirectData;
	inIndirectData.DSV = GetCurrentDSV();
	inIndirectData.DSV_CPU = GetCurrentDSV_CPUAddr();
	for (auto& gBufferPtr : m_pGBuffers) inIndirectData.RTV.push_back(gBufferPtr.get());
	inIndirectData.RTV_CPU = m_pGBuffersRTVPack->GetResult()->GetCPUDescriptorHandle(0);
	inIndirectData.Rect = sceneRect;
	inIndirectData.Viewport = sceneViewPort;
	auto indirectRenderH = m_pRenderMeshesSystem->IndirectRender({meshH, HiZUpdateH, preDepthH, cameraH, animationGpuSkinningH }, inIndirectData);


	std::shared_ptr<FD3DW::ExecutionHandle> atlasRtShadowsH = nullptr;
	if (IsRTSupported()) {

		atlasRtShadowsH = m_pAtlasRTShadowSystem->OnGenerateShadowAtlas({ clusterAssignH, tlasCallH, lightsH, uploadMetaH, indirectRenderH });
	
	}

	//////////////////////////
	// SHADING PASS
	auto gSecondPassRecipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_DIRECT, [this, sceneRect, sceneViewPort](ID3D12GraphicsCommandList* list) {
		list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_pForwardRenderPassRTV->StartDraw(list);

		list->RSSetScissorRects(1, &sceneRect);
		list->RSSetViewports(1, &sceneViewPort);

		list->ClearRenderTargetView(m_pForwardRenderPassRTVPack->GetResult()->GetCPUDescriptorHandle(0), COLOR, 0, nullptr);
		list->OMSetRenderTargets(1, &FD3DW::keep(m_pForwardRenderPassRTVPack->GetResult()->GetCPUDescriptorHandle(0)), true, &FD3DW::keep(GetCurrentDSV_CPUAddr()));

		PSOManager::GetInstance()->GetPSOObject(PSOType::DefferedSecondPassDefaultConfig)->Bind(list);

		m_pGBuffersSRVPack->AddResource(GetCurrentDSV(), DEPTH_BUFFER_LOCATION_IN_HEAP, GetDevice());
		list->IASetVertexBuffers(0, 1, m_pSceneVBV_IBV->GetVertexBufferView());
		list->IASetIndexBuffer(m_pSceneVBV_IBV->GetIndexBufferView());

		ID3D12DescriptorHeap* heaps[] = { m_pGBuffersSRVPack->GetResult()->GetDescriptorPtr() };
		list->SetDescriptorHeaps(_countof(heaps), heaps);

		list->SetGraphicsRootDescriptorTable(DEFFERED_GBUFFERS_POS_IN_ROOT_SIG, m_pGBuffersSRVPack->GetResult()->GetGPUDescriptorHandle(0));

		list->SetGraphicsRootConstantBufferView(LIGHTS_HELPER_BUFFER_POS_IN_ROOT_SIG, m_pLightSystem->GetLightsConstantBufferGPULocation());
		list->SetGraphicsRootShaderResourceView(LIGHTS_BUFFER_POS_IN_ROOT_SIG, m_pLightSystem->GetLightsStructuredBufferGPULocation());
		list->SetGraphicsRootShaderResourceView(LIGHTS_CLUSTERS_BUFFER_POS_IN_ROOT_SIG, m_pClusteredLightningSystem->GetClusteredStructuredBufferGPULocation());
		list->SetGraphicsRootConstantBufferView(LIGHTS_CLUSTERS_DATA_BUFFER_POS_IN_ROOT_SIG, m_pClusteredLightningSystem->GetClusteredConstantBufferGPULocation());
		list->SetGraphicsRootShaderResourceView(LIGHTS_SHADOWS_ATLAS_LIGHT_METAS_POS_IN_ROOT_SIG, m_pAtlasRTShadowSystem->GetLightsMetasBufferGPULocation());
		list->SetGraphicsRootConstantBufferView(LIGHTS_SHADOWS_ATLAS_CBV_PARAMS_POS_IN_ROOT_SIG, m_pAtlasRTShadowSystem->GetAtlasConstantBufferGPULocation());

		list->DrawIndexedInstanced(GetIndexSize(m_pScreen.get(), 0), 1, GetIndexStartPos(m_pScreen.get(), 0), GetVertexStartPos(m_pScreen.get(), 0), 0);

		m_pForwardRenderPassRTV->EndDraw(list);
	});

	auto shadingPassH = GlobalRenderThreadManager::GetInstance()->Submit(gSecondPassRecipe, { lightsH, clusterAssignH, indirectRenderH, atlasRtShadowsH });

	SkyboxRenderPassInput inSkyboxRenderData;
	inSkyboxRenderData.RTV = m_pForwardRenderPassRTV.get();
	inSkyboxRenderData.RTV_CPU = m_pForwardRenderPassRTVPack->GetResult()->GetCPUDescriptorHandle(0);
	inSkyboxRenderData.DSV_CPU = GetCurrentDSV_CPUAddr();
	inSkyboxRenderData.Rect = sceneRect;
	inSkyboxRenderData.Viewport = sceneViewPort;
	auto skyboxRenderH = m_pSkyboxRenderSystem->RenderSkyboxPass(shadingPassH, inSkyboxRenderData);

	auto taaPassH = m_pTAASystem->ProcessTAABufferCollection({ skyboxRenderH });

	auto bloomPassH = m_pBloomEffectSystem->ProcessBloomPass({ skyboxRenderH , taaPassH }, GetShadingResultResource());


	auto gPostProcessPass = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_DIRECT, [this, sceneRect, sceneOutputViewPort, mainViewPort, mainRect](ID3D12GraphicsCommandList* list) {
		//////////////////////////
		// POSTPROCESS PASS	
		list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		list->RSSetViewports(1, &mainViewPort);
		list->RSSetScissorRects(1, &mainRect);
		BeginDraw(list);

		m_pForwardRenderPassSRVPack->AddResource(GetShadingResultResource()->GetResource(), D3D12_SRV_DIMENSION_TEXTURE2D, 0u, GetDevice());

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

			list->RSSetViewports(1, &sceneOutputViewPort);
			list->RSSetScissorRects(1, &sceneRect);

			list->DrawIndexedInstanced(GetIndexSize(m_pScreen.get(), 0), 1, GetIndexStartPos(m_pScreen.get(), 0), GetVertexStartPos(m_pScreen.get(), 0), 0);

		}

		/////
		/////////////////////////////


		////////////////////////////
		////			UI PASS

		{
			list->RSSetViewports(1, &mainViewPort);
			list->RSSetScissorRects(1, &mainRect);
				m_pUIComponent->RenderImGui(list);
			}

		/////
		/////////////////////////////

		{
			EndDraw(list);
		}
	});

	auto postProcessPass = GlobalRenderThreadManager::GetInstance()->Submit(gPostProcessPass, { shadingPassH, skyboxRenderH, bloomPassH, taaPassH });


	auto lCameraH = m_pCameraSystem->OnEndTick(indirectRenderH);
	auto lMeshH = m_pRenderMeshesSystem->OnEndRenderTick({ indirectRenderH });

	auto presentH = GlobalRenderThreadManager::GetInstance()->SubmitLambda([this]() { 
		PresentSwapchain(); 
		++m_uFrameIndex;
		m_uCurrentDSVIndex = (m_uCurrentDSVIndex + 1) % 2;
	}, { postProcessPass,lCameraH,lMeshH }, true);

	m_dInFlight.push_back(presentH);

	auto needProcessUI = m_pUIComponent->GetPendingAfterRenderCallsCount()>0;
	while ( ( needProcessUI && !m_dInFlight.empty() ) || m_dInFlight.size() >= m_uMaxFramesInFlight) {
		auto front = m_dInFlight.front();
		if (front && !front->IsDone()) {
			front->WaitForExecute();
		}
		m_dInFlight.pop_front();
	}

	if(needProcessUI) m_pUIComponent->ProcessAfterRenderUICalls();
	ProcessNotifiesInWorld();

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

bool MainRenderer::IsEnabledTAA() {
	return m_pTAASystem && m_pTAASystem->IsTAAEnabled();
}

void MainRenderer::EnableTAA(bool b) {
	if (m_pTAASystem) {
		m_pTAASystem->EnableTAA(b);
	}

	if (m_bLinkJitterToTAA && m_pCameraSystem) {
		m_pCameraSystem->EnableJitter(b);
	}
}

bool MainRenderer::IsEnabledJitter() {
	return m_pCameraSystem && m_pCameraSystem->IsJitterEnabled();
}

void MainRenderer::EnableJitter(bool b) {
	if (m_pCameraSystem) {
		m_pCameraSystem->EnableJitter(b);
	}
}

bool MainRenderer::IsLinkJitterToTAAEnabled() const {
	return m_bLinkJitterToTAA;
}

void MainRenderer::EnableLinkJitterToTAA(bool b) {
	m_bLinkJitterToTAA = b;
	if (m_bLinkJitterToTAA && m_pCameraSystem && m_pTAASystem) {
		m_pCameraSystem->EnableJitter(m_pTAASystem->IsTAAEnabled());
	}
}

FLOAT* MainRenderer::GetClearColor(){
	return COLOR;
}

World* MainRenderer::GetWorld() {
	return m_pWorld.get();
}

void MainRenderer::SetSelectedEntity(ComponentHolder* entity) {
	if (entity && m_pWorld) {
		if( !IsEntityAlive(entity) ) entity = nullptr;
	}

	m_pSelectedEntity = entity;
	if(!m_pSelectedEntity || (m_pSelectedMeshComponent && m_pSelectedMeshComponent->GetOwner() != m_pSelectedEntity)) {
		m_pSelectedMeshComponent = nullptr;
	}
}

bool MainRenderer::RayIntersectsAABB(const dx::XMFLOAT3& rayOrigin, const dx::XMFLOAT3& rayDir, const dx::XMFLOAT3& boundsMin, const dx::XMFLOAT3& boundsMax, float& outDistance) {
	auto tMin = 0.0f;
	auto tMax = std::numeric_limits<float>::max();

	float originValues[3] = { rayOrigin.x, rayOrigin.y, rayOrigin.z };
	float dirValues[3] = { rayDir.x, rayDir.y, rayDir.z };
	float minValues[3] = { boundsMin.x, boundsMin.y, boundsMin.z };
	float maxValues[3] = { boundsMax.x, boundsMax.y, boundsMax.z };

	for (auto axis = 0; axis < 3; ++axis) {
		auto origin = originValues[axis];
		auto dir = dirValues[axis];
		auto minBound = minValues[axis];
		auto maxBound = maxValues[axis];

		if(std::abs(dir) < EPS_F) {
			if (origin < minBound || origin > maxBound) return false;

			continue;
		}

		auto invDir = 1.0f / dir;
		auto t1 = (minBound - origin) * invDir;
		auto t2 = (maxBound - origin) * invDir;
		if (t1 > t2) std::swap(t1, t2);

		tMin = std::max(tMin, t1);
		tMax = std::min(tMax, t2);

		if (tMin > tMax) return false;
	}

	outDistance = tMin >= 0.0f ? tMin : tMax;
	return outDistance >= 0.0f;
}

ComponentHolder* MainRenderer::GetSelectedEntity() const {
	return m_pSelectedEntity;
}

void MainRenderer::SetSelectedMeshComponent(MeshComponent* component) {
	if (component && m_pWorld) {
		if (! IsComponentAlive(component) ) component = nullptr;

		
	}

	m_pSelectedMeshComponent = component;
	if (m_pSelectedMeshComponent) {
		m_pSelectedEntity = m_pSelectedMeshComponent->GetOwner();
	}
}

MeshComponent* MainRenderer::GetSelectedMeshComponent() const {
	return m_pSelectedMeshComponent;
}

void MainRenderer::SelectEntityAtScreenPoint(int mouseX, int mouseY) {
	auto pickedMeshComponent = PickMeshComponentAtScreenPoint(mouseX, mouseY);
	SetSelectedMeshComponent(pickedMeshComponent);
	SetSelectedEntity(pickedMeshComponent ? pickedMeshComponent->GetOwner() : nullptr);
}

ComponentHolder* MainRenderer::PickEntityAtScreenPoint(int mouseX, int mouseY) {
	auto meshComponent = PickMeshComponentAtScreenPoint(mouseX, mouseY);
	return meshComponent ? meshComponent->GetOwner() : nullptr;
}

MeshComponent* MainRenderer::PickMeshComponentAtScreenPoint(int mouseX, int mouseY) {
	if (!m_pWorld || mouseX < 0 || mouseY < 0) return nullptr;

	auto sceneWidth = GetSceneRenderWidth();
	auto sceneHeight = GetSceneRenderHeight();
	if (sceneWidth <= 0 || sceneHeight <= 0) return nullptr;
	
	if (mouseX < m_xSceneRect.left || mouseX >= m_xSceneRect.right) return nullptr;
	
	if (mouseY < m_xSceneRect.top || mouseY >= m_xSceneRect.bottom) return nullptr;

	auto view = GetCurrentViewMatrix();
	auto projection = GetCurrentProjectionMatrix();
	auto identity = dx::XMMatrixIdentity();

	auto nearPoint = dx::XMVector3Unproject(
		dx::XMVectorSet((float)mouseX, (float)mouseY, 0.0f, 1.0f),
		m_xSceneViewPort.TopLeftX,
		m_xSceneViewPort.TopLeftY,
		m_xSceneViewPort.Width,
		m_xSceneViewPort.Height,
		0.0f,
		1.0f,
		projection,
		view,
		identity
	);
	auto farPoint = dx::XMVector3Unproject(
		dx::XMVectorSet((float)mouseX, (float)mouseY, 1.0f, 1.0f),
		m_xSceneViewPort.TopLeftX,
		m_xSceneViewPort.TopLeftY,
		m_xSceneViewPort.Width,
		m_xSceneViewPort.Height,
		0.0f,
		1.0f,
		projection,
		view,
		identity
	);

	dx::XMFLOAT3 rayOrigin;
	dx::XMFLOAT3 rayDirection;
	dx::XMStoreFloat3(&rayOrigin, nearPoint);
	dx::XMStoreFloat3(&rayDirection, dx::XMVector3Normalize(farPoint - nearPoint));

	MeshComponent* pickedMeshComponent = nullptr;
	auto closestDistance = std::numeric_limits<float>::max();

	auto meshComponents = m_pWorld->GetAllComponentsOfType<MeshComponent>();
	for(auto meshComponent : meshComponents) {
		if(!meshComponent || !meshComponent->IsActive()) continue;

		auto owner = meshComponent->GetOwner();
		if(!owner || !owner->IsActive()) continue;

		auto [boundsMin, boundsMax] = meshComponent->GetWorldBounds();
		auto hitDistance = 0.0f;
		if (!RayIntersectsAABB(rayOrigin, rayDirection, boundsMin, boundsMax, hitDistance)) continue;

		if (hitDistance < closestDistance) {
			closestDistance = hitDistance;
			pickedMeshComponent = meshComponent;
		}
	}

	return pickedMeshComponent;
}

D3D12_RECT MainRenderer::GetSceneRect() const {
	return m_xSceneRect;
}

D3D12_VIEWPORT MainRenderer::GetSceneViewport() const {
	return m_xSceneViewPort;
}

int MainRenderer::GetSceneRenderWidth() const {
	auto width = int(m_xSceneRect.right - m_xSceneRect.left);
	return std::max(0, width);
}

int MainRenderer::GetSceneRenderHeight() const {
	auto height = int(m_xSceneRect.bottom - m_xSceneRect.top);
	return std::max(0, height);
}

void MainRenderer::RequestSceneViewportLayoutUpdate() {
	auto prevSceneWidth = GetSceneRenderWidth();
	auto prevSceneHeight = GetSceneRenderHeight();
	
	UpdateSceneViewportLayout();
	
	auto newSceneWidth = GetSceneRenderWidth();
	auto newSceneHeight = GetSceneRenderHeight();

	if (newSceneWidth != prevSceneWidth || newSceneHeight != prevSceneHeight) {
		RecreateWindowSizeDependentResources(newSceneWidth, newSceneHeight);
	}
}

FD3DW::DepthStencilView* MainRenderer::GetCurrentDSV() {
	return m_pDSV[m_uCurrentDSVIndex].get();
}

D3D12_CPU_DESCRIPTOR_HANDLE MainRenderer::GetCurrentDSV_CPUAddr() {
	return m_pDSVPack->GetResult()->GetCPUDescriptorHandle(m_uCurrentDSVIndex);
}

D3D12_GPU_DESCRIPTOR_HANDLE MainRenderer::GetCurrentDSV_GPUAddr() {
	return m_pDSVPack->GetResult()->GetGPUDescriptorHandle(m_uCurrentDSVIndex);
}

UINT MainRenderer::GetCurrentDSVIndex() {
	return m_uCurrentDSVIndex;
}

dx::XMFLOAT2 MainRenderer::GetCurrentJitterOffset() const {
	return m_pCameraSystem->GetJitterOffset();
}

dx::XMFLOAT2 MainRenderer::GetPrevJitterOffset() const {
	return m_pCameraSystem->GetPrevJitterOffset();
}

dx::XMMATRIX MainRenderer::GetCurrentProjectionMatrix() const {
	return m_pCameraSystem->GetProjectionMatrix();
}

dx::XMMATRIX MainRenderer::GetCurrentJitteredProjectionMatrix() const {
	return m_pCameraSystem->GetJitteredProjectionMatrix();
}

dx::XMMATRIX MainRenderer::GetCurrentViewMatrix() const {
	return m_pCameraSystem->GetViewMatrix();
}

dx::XMMATRIX MainRenderer::GetViewProjectionMatrix() const {
	return m_pCameraSystem->GetViewProjectionMatrix();
}

dx::XMMATRIX MainRenderer::GetJitteredViewProjectionMatrix() const {
	return m_pCameraSystem->GetJitteredViewProjectionMatrix();
}

dx::XMMATRIX MainRenderer::GetPrevViewProjectionMatrix() const {
	return m_pCameraSystem->GetPrevViewProjectionMatrix();
}

dx::XMFLOAT3 MainRenderer::GetCurrentCameraPosition() const {
	return m_pCameraSystem->GetCameraPosition();
}

CameraFrustum MainRenderer::GetCameraFrustum() {
	return m_pCameraSystem->GetCameraFrustum();
}

float MainRenderer::GetFoVY() const {
	return m_pCameraSystem->GetFoVY();
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

const std::vector<LightComponentData>& MainRenderer::GetLightComponentsData() const {
	return m_pLightSystem->GetLightComponentsData();
}

int MainRenderer::GetLightsCount() {
	return m_pLightSystem->GetLightsCount();
}

bool MainRenderer::IsShadowEnabled() {
	return IsRTSupported();
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

	m_pSelectedEntity = nullptr;
	m_pSelectedMeshComponent = nullptr;
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

FD3DW::FResource* MainRenderer::GetShadingResultResource() {
	return (!m_pTAASystem || !m_pTAASystem->IsTAAEnabled()) ? m_pForwardRenderPassRTV->GetTexture() : m_pTAASystem->GetCurrentResultResource();
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

void MainRenderer::ProcessNotifies(std::vector<NRenderSystemNotifyType> notifies) {
	for (auto& sys : m_vSystems) {
		for (auto notify : notifies) {
			sys->ProcessNotify(notify);
		}
	}
}

void MainRenderer::ValidateSelection() {
	if (!m_pWorld) {
		m_pSelectedEntity = nullptr;
		m_pSelectedMeshComponent = nullptr;
		return;
	}

	if (m_pSelectedEntity && !IsEntityAlive(m_pSelectedEntity)) {
		m_pSelectedEntity = nullptr;
	}

	if (m_pSelectedMeshComponent && !IsComponentAlive(m_pSelectedMeshComponent)) {
		m_pSelectedMeshComponent = nullptr;
	}

	if (!m_pSelectedEntity) {
		m_pSelectedMeshComponent = nullptr;
	}
	else if (m_pSelectedMeshComponent && m_pSelectedMeshComponent->GetOwner() != m_pSelectedEntity) {
		m_pSelectedMeshComponent = nullptr;
	}
}

void MainRenderer::UpdateSceneViewportLayout() {
	const auto& wndSettings = WNDSettings();
	auto wndWidth = std::max(1, (int)wndSettings.Width);
	auto wndHeight = std::max(1, (int)wndSettings.Height);
	auto prevSceneWidth = GetSceneRenderWidth();
	auto prevSceneHeight = GetSceneRenderHeight();

	auto sceneRight = wndWidth;
	
	if (m_pUIComponent && m_pUIComponent->IsUIVisible()) {
		auto maxPanelWidth = std::max(0, wndWidth - WINDOW_MINIMAL_SCENE_WIDTH);
		auto minPanelWidth = std::min(WINDOW_MINIMAL_UI_WIDTH, maxPanelWidth);
		auto panelWidth = std::clamp( (int)std::lround(m_pUIComponent->GetFixedPanelWidth()), minPanelWidth, maxPanelWidth );
		sceneRight = wndWidth - panelWidth;
	}

	sceneRight = std::clamp(sceneRight, 1, wndWidth);

	m_xSceneRect.left = 0;
	m_xSceneRect.top = 0;
	m_xSceneRect.right = sceneRight;
	m_xSceneRect.bottom = wndHeight;

	m_xSceneViewPort.TopLeftX = (float)m_xSceneRect.left;
	m_xSceneViewPort.TopLeftY = (float)m_xSceneRect.top;
	m_xSceneViewPort.Width = (float)GetSceneRenderWidth();
	m_xSceneViewPort.Height = (float)GetSceneRenderHeight();
	m_xSceneViewPort.MinDepth = 0.0f;
	m_xSceneViewPort.MaxDepth = 1.0f;

	m_xSceneOutputViewPort.TopLeftX = (float)m_xSceneRect.left;
	m_xSceneOutputViewPort.TopLeftY = (float)m_xSceneRect.top;
	m_xSceneOutputViewPort.Width = (float)GetSceneRenderWidth();
	m_xSceneOutputViewPort.Height = (float)GetSceneRenderHeight();
	m_xSceneOutputViewPort.MinDepth = 0.0f;
	m_xSceneOutputViewPort.MaxDepth = 1.0f;

	m_xMainViewPort.TopLeftX = 0.0f;
	m_xMainViewPort.TopLeftY = 0.0f;
	m_xMainViewPort.Width = (float)wndWidth;
	m_xMainViewPort.Height = (float)wndHeight;
	m_xMainViewPort.MinDepth = 0.0f;
	m_xMainViewPort.MaxDepth = 1.0f;

	m_xMainRect.left = 0;
	m_xMainRect.top = 0;
	m_xMainRect.right = (LONG)wndWidth;
	m_xMainRect.bottom = (LONG)wndHeight;

	if (m_pCameraSystem) {
		auto newSceneWidth = GetSceneRenderWidth();
		auto newSceneHeight = GetSceneRenderHeight();
		if (newSceneWidth != prevSceneWidth || newSceneHeight != prevSceneHeight) {
			m_pCameraSystem->OnResizeWindow();
		}
	}
}

bool MainRenderer::IsEntityAlive(ComponentHolder* entity) {
	if (!m_pWorld || !entity) return false;
	auto entities = m_pWorld->GetEntities();
	return std::find(entities.begin(), entities.end(), entity) != entities.end();
}

bool MainRenderer::IsComponentAlive(MeshComponent* cmp) {
	if (!m_pWorld || !cmp) return false;
	const auto components = m_pWorld->GetAllComponentsOfType<MeshComponent>();
	return std::find(components.begin(), components.end(), cmp) != components.end();
}

void MainRenderer::OnMainWindowResize(int width, int height) {
	if (width <= 0 || height <= 0) return;

	UpdateSceneViewportLayout();
	RecreateWindowSizeDependentResources( GetSceneRenderWidth(), GetSceneRenderHeight() );
}

void MainRenderer::RecreateWindowSizeDependentResources(int width, int height) {
	for (auto& handle : m_dInFlight) {
		if ( handle && !handle->IsDone() ) {
			handle->WaitForExecute();
		}
	}
	m_dInFlight.clear();
	GlobalRenderThreadManager::GetInstance()->WaitIdle();

	auto device = GetDevice();
	m_uCurrentDSVIndex = 0;

	m_pDSVPack = CreateDSVPack(2u);
	for (int i = 0; i < 2; ++i) {
		m_pDSV[i] = CreateDepthStencilView(DXGI_FORMAT_R24G8_TYPELESS, DXGI_FORMAT_D24_UNORM_S8_UINT, DXGI_FORMAT_R24_UNORM_X8_TYPELESS, D3D12_DSV_DIMENSION_TEXTURE2D, 1, width, height);
		m_pDSVPack->PushResource(m_pDSV[i]->GetResource(), m_pDSV[i]->GetDSVDesc(), device);
	}

	const auto& gBufferFormats = GetGBufferData().GBuffersFormats;
	const auto gbuffersNum = (UINT)gBufferFormats.size();

	m_pGBuffers.clear();
	m_pGBuffersRTVPack = CreateRTVPack(gbuffersNum);
	m_pGBuffersSRVPack = CreateSRVPack(COUNT_SRV_IN_GBUFFER_HEAP);
	m_pGBuffersSRVPack->AddNullResource(SHADOW_FACTOR_LOCATION_IN_HEAP, device);

	for (const auto& format : gBufferFormats) {
		auto gbuffer = CreateRenderTarget(format, D3D12_RTV_DIMENSION_TEXTURE2D, 1, width, height);
		m_pGBuffersRTVPack->PushResource(gbuffer->GetRTVResource(), gbuffer->GetRTVDesc(), device);
		m_pGBuffersSRVPack->PushResource(device, gbuffer->GetRTVResource(), D3D12_SRV_DIMENSION_TEXTURE2D);
		m_pGBuffers.push_back(std::move(gbuffer));
	}

	m_pGBuffersSRVPack->AddResource(GetCurrentDSV(), DEPTH_BUFFER_LOCATION_IN_HEAP, device);
	if (m_vLCTResources.size() > 0 && m_vLCTResources[0]) {
		m_pGBuffersSRVPack->AddResource(m_vLCTResources[0]->GetResource(), D3D12_SRV_DIMENSION_TEXTURE2D, LIGHTS_LTC_MAT_LOCATION_IN_HEAP, device);
	}
	if (m_vLCTResources.size() > 1 && m_vLCTResources[1]) {
		m_pGBuffersSRVPack->AddResource(m_vLCTResources[1]->GetResource(), D3D12_SRV_DIMENSION_TEXTURE2D, LIGHTS_LTC_AMP_LOCATION_IN_HEAP, device);
	}

	m_pForwardRenderPassRTV = CreateRenderTarget(GetForwardRenderPassFormat(), D3D12_RTV_DIMENSION_TEXTURE2D, 1, width, height);
	m_pForwardRenderPassRTVPack = CreateRTVPack(1u);
	m_pForwardRenderPassSRVPack = CreateSRVPack(2u);
	m_pForwardRenderPassRTVPack->PushResource(m_pForwardRenderPassRTV->GetRTVResource(), m_pForwardRenderPassRTV->GetRTVDesc(), device);
	m_pForwardRenderPassSRVPack->PushResource(device, m_pForwardRenderPassRTV->GetRTVResource(), D3D12_SRV_DIMENSION_TEXTURE2D);

	if (m_pBloomEffectSystem) {
		m_pBloomEffectSystem->ResizeResources((UINT)width, (UINT)height);
		m_pForwardRenderPassSRVPack->AddResource(m_pBloomEffectSystem->GetResultResource(), D3D12_SRV_DIMENSION_TEXTURE2D, 1, device);
	}

	if (m_pTAASystem) {
		m_pTAASystem->ResizeResources((UINT)width, (UINT)height);
		m_pTAASystem->SetGBufferResources(m_pForwardRenderPassRTV->GetTexture(), m_pGBuffers[GBUFFER_MOTIONVECTORDATA_LOCATION_IN_HEAP]->GetTexture(), m_pDSV[0].get(), m_pDSV[1].get());
	}

	if (IsRTSupported() && m_pAtlasRTShadowSystem) {
		m_pAtlasRTShadowSystem->SetGBuffersResources(m_pGBuffers[GBUFFER_NORMAL_LOCATION_IN_HEAP]->GetTexture(), device);
		m_pGBuffersSRVPack->AddResource(m_pAtlasRTShadowSystem->GetShadowAtlas()->GetResource(), D3D12_SRV_DIMENSION_TEXTURE2D, SHADOW_FACTOR_LOCATION_IN_HEAP, device);
	}

	GlobalRenderThreadManager::GetInstance()->WaitIdle();
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
	m_pForwardRenderPassSRVPack->AddResource(m_pBloomEffectSystem->GetResultResource(), D3D12_SRV_DIMENSION_TEXTURE2D, 1, device);

	m_pTAASystem = CreateSystem<TAASystem>();
	m_pTAASystem->SetGBufferResources(m_pForwardRenderPassRTV->GetTexture(), m_pGBuffers[GBUFFER_MOTIONVECTORDATA_LOCATION_IN_HEAP]->GetTexture(), m_pDSV[0].get(), m_pDSV[1].get());
	EnableLinkJitterToTAA(m_bLinkJitterToTAA);
	EnableTAA(m_pTAASystem->IsTAAEnabled());

}

void MainRenderer::InitMainRendererDXRSystems(ID3D12Device5* device) {
	m_pAtlasRTShadowSystem = CreateSystem<AtlasRTShadowSystem>();
	m_pAtlasRTShadowSystem->SetGBuffersResources(m_pGBuffers[GBUFFER_NORMAL_LOCATION_IN_HEAP]->GetTexture(), device);
	m_pGBuffersSRVPack->AddResource(m_pAtlasRTShadowSystem->GetShadowAtlas()->GetResource(), D3D12_SRV_DIMENSION_TEXTURE2D, SHADOW_FACTOR_LOCATION_IN_HEAP, device);
}
