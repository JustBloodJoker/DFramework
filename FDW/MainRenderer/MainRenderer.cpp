#include <MainRenderer/MainRenderer.h>
#include <MainRenderer/PSOManager.h>
#include <MainRenderer/GlobalTextureHeap.h>
#include <D3DFramework/Utilites/Serializer/BinarySerializer.h>
#include <D3DFramework/GraphicUtilites/CommandList.h>

#include <D3DFramework/Objects/RTObjectHelper.h>
#include <D3DFramework/GraphicUtilites/RTPipelineObject.h>
#include <D3DFramework/GraphicUtilites/RTShaderBindingTable.h>

#include <Lights/MainRenderer_RTSoftShadowsComponent.h>

static FLOAT COLOR[4] = { 0.2f,0.2f,0.2f,1.0f };

MainRenderer::MainRenderer() : WinWindow(L"FDW", 1024, 1024, false) {}

void MainRenderer::UserInit()
{
	auto device = GetDevice();
	auto dxrDevice = GetDXRDevice();

	InitMainRendererParts(device);
	if(dxrDevice) InitMainRendererDXRParts(dxrDevice);

	InitMainRendererComponents();

	SetVSync(true);

	m_pDSV = CreateDepthStencilView(DXGI_FORMAT_R24G8_TYPELESS, DXGI_FORMAT_D24_UNORM_S8_UINT, DXGI_FORMAT_R24_UNORM_X8_TYPELESS, D3D12_DSV_DIMENSION_TEXTURE2D, 1, 1024, 1024);
	
	m_pDSVPack = CreateDSVPack(1u);
	m_pDSVPack->PushResource(m_pDSV->GetResource(), m_pDSV->GetDSVDesc(), device);

	auto wndSettings = GetMainWNDSettings();

	const auto& gBufferFormats = GetGBufferData().GBuffersFormats;
	auto gbuffersNum = (UINT)gBufferFormats.size();
	m_pGBuffersRTVPack = CreateRTVPack(gbuffersNum);
	m_pGBuffersSRVPack = CreateSRVPack(COUNT_SRV_IN_GBUFFER_HEAP);

	for (const auto& format : gBufferFormats) {
		m_pGBuffers.push_back(CreateRenderTarget(format, D3D12_RTV_DIMENSION_TEXTURE2D, 1, wndSettings.Width, wndSettings.Height));
		auto& gbuffer = m_pGBuffers.back();
		
		m_pGBuffersRTVPack->PushResource(gbuffer->GetRTVResource(), gbuffer->GetRTVDesc(), device);
		m_pGBuffersSRVPack->PushResource(device, gbuffer->GetRTVResource(), D3D12_SRV_DIMENSION_TEXTURE2D);
	}

	auto ltcRecipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_DIRECT, [this](ID3D12GraphicsCommandList* list) {
		m_pLightsManager->InitLTC(list, m_pGBuffersSRVPack.get());
	});
	GlobalRenderThreadManager::GetInstance()->Submit(ltcRecipe);
	
	m_pForwardRenderPassRTV = CreateRenderTarget(GetForwardRenderPassFormat(), D3D12_RTV_DIMENSION_TEXTURE2D, 1, wndSettings.Width, wndSettings.Height);
	m_pForwardRenderPassRTVPack = CreateRTVPack(1u);
	m_pForwardRenderPassSRVPack = CreateSRVPack(1u);
	m_pForwardRenderPassRTVPack->PushResource(m_pForwardRenderPassRTV->GetRTVResource(), m_pForwardRenderPassRTV->GetRTVDesc(), device);
	m_pForwardRenderPassSRVPack->PushResource(device, m_pForwardRenderPassRTV->GetRTVResource(), D3D12_SRV_DIMENSION_TEXTURE2D);

	auto rtvRectangleCreation = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_DIRECT, [this](ID3D12GraphicsCommandList* list) {
		m_pScreen = CreateRectangle(list);
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

	TryInitShadowComponent();

	GlobalRenderThreadManager::GetInstance()->WaitIdle();
	FD3DW::FResource::ReleaseUploadBuffers();
}

void MainRenderer::UserLoop()
{
	auto beforeRenderCopyRecipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_DIRECT, [this](ID3D12GraphicsCommandList* list) {
		m_pLightsManager->BeforeRender(list);
		m_pRenderableObjectsManager->BeforeRender(list);
		if (m_pShadowsComponent) m_pShadowsComponent->BeforeRender(list);
	});
	auto beforeRenderH = GlobalRenderThreadManager::GetInstance()->Submit(beforeRenderCopyRecipe);


	///////////////////////
	//	Shadow PASS Before gbuffer pass
	{
		if (m_pShadowsComponent) {
			m_pShadowsComponent->BeforeGBufferPass();
		}
	}
	///	
	///////////////////////////
	auto clusterGenerationsH = m_pLightsManager->ClusteredShadingPass(beforeRenderH);


	std::shared_ptr<FD3DW::ExecutionHandle> preDepthH = beforeRenderH;
	if (m_bIsEnabledPreDepth)
	{
		///////////////////////
		//	PRE DEPTH PASS
		auto preDepthPassRecipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_DIRECT, [this](ID3D12GraphicsCommandList* list) {
			list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			{
				if (m_bIsEnabledPreDepth)
				{
					m_pDSV->DepthWrite(list);
					list->ClearDepthStencilView(m_pDSVPack->GetResult()->GetCPUDescriptorHandle(0), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

					PSOManager::GetInstance()->GetPSOObject(PSOType::PreDepthDefaultConfig)->Bind(list);
					list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

					list->RSSetScissorRects(1, &m_xSceneRect);
					list->RSSetViewports(1, &m_xSceneViewPort);
					list->OMSetRenderTargets(0, nullptr, false, &FD3DW::keep(m_pDSVPack->GetResult()->GetCPUDescriptorHandle(0)));

					m_pRenderableObjectsManager->PreDepthRender(list);
				}
			}
		});
		///////////////////////
		preDepthH = GlobalRenderThreadManager::GetInstance()->Submit(preDepthPassRecipe, { beforeRenderH });
	}

	auto HiZUpdateH = preDepthH;
	if (m_pRenderableObjectsManager->GetMeshCullingType() == CullingType::GPUCulling) {
		auto updateHiZ = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_COMPUTE, [this](ID3D12GraphicsCommandList* list) {
			m_pRenderableObjectsManager->UpdateHiZResource(list);
		});
		HiZUpdateH = GlobalRenderThreadManager::GetInstance()->Submit(updateHiZ, { preDepthH });
	}

	auto gBuffersPassRecipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_DIRECT, [this](ID3D12GraphicsCommandList* list) {		
		if (!m_bIsEnabledPreDepth) 
		{
			m_pDSV->DepthWrite(list);
			list->ClearDepthStencilView(m_pDSVPack->GetResult()->GetCPUDescriptorHandle(0), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
		}
		else
		{
			m_pDSV->DepthRead(list);
		}

		///////////////////////
		//	DEFERRED FIRST PASS
		{
			list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			auto gBuffersCount = GetGBuffersNum();

			for (auto& gbuffer : m_pGBuffers) {
				gbuffer->StartDraw(list);
			}

			list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			list->RSSetScissorRects(1, &m_xSceneRect);
			list->RSSetViewports(1, &m_xSceneViewPort);

			for (UINT i = 0; i < gBuffersCount; ++i) {
				list->ClearRenderTargetView(m_pGBuffersRTVPack->GetResult()->GetCPUDescriptorHandle(i), COLOR, 0, nullptr);
			}
			list->OMSetRenderTargets(gBuffersCount, &FD3DW::keep(m_pGBuffersRTVPack->GetResult()->GetCPUDescriptorHandle(0)), true, &FD3DW::keep(m_pDSVPack->GetResult()->GetCPUDescriptorHandle(0)));

			m_pRenderableObjectsManager->DeferredRender(list);

			for (auto& gbuffer : m_pGBuffers) {
				gbuffer->EndDraw(list);
			}

		}

		///
		///////////////////////////
	});
	auto gBufferH = GlobalRenderThreadManager::GetInstance()->Submit(gBuffersPassRecipe,{ HiZUpdateH });
	
	///////////////////////
	//	Shadow PASS After GBuffer pass
	{
		if (m_pShadowsComponent) {
			m_pShadowsComponent->AfterGBufferPass();
		}
	}
	///
	///////////////////////////
	
	auto shadowsH = GlobalRenderThreadManager::GetInstance()->CreateWaitHandle(D3D12_COMMAND_LIST_TYPE_DIRECT);

	auto gSecondPassRecipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_DIRECT, [this](ID3D12GraphicsCommandList* list) {

		//////////////////////////
		// DEFERRED SECOND PASS

		{
			list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			m_pForwardRenderPassRTV->StartDraw(list);

			if(m_pShadowsComponent) m_pShadowsComponent->OnDrawResource(list);

			list->RSSetScissorRects(1, &m_xSceneRect);
			list->RSSetViewports(1, &m_xSceneViewPort);

			list->ClearRenderTargetView(m_pForwardRenderPassRTVPack->GetResult()->GetCPUDescriptorHandle(0), COLOR, 0, nullptr);
			list->OMSetRenderTargets(1, &FD3DW::keep(m_pForwardRenderPassRTVPack->GetResult()->GetCPUDescriptorHandle(0)), true, &FD3DW::keep(m_pDSVPack->GetResult()->GetCPUDescriptorHandle(0)));

			PSOManager::GetInstance()->GetPSOObject(PSOType::DefferedSecondPassDefaultConfig)->Bind(list);

			list->IASetVertexBuffers(0, 1, m_pScreen->GetVertexBufferView());
			list->IASetIndexBuffer(m_pScreen->GetIndexBufferView());

			ID3D12DescriptorHeap* heaps[] = { m_pGBuffersSRVPack->GetResult()->GetDescriptorPtr() };
			list->SetDescriptorHeaps(_countof(heaps), heaps);

			list->SetGraphicsRootDescriptorTable(DEFFERED_GBUFFERS_POS_IN_ROOT_SIG, m_pGBuffersSRVPack->GetResult()->GetGPUDescriptorHandle(0));

			m_pLightsManager->BindLightConstantBuffer(LIGHTS_HELPER_BUFFER_POS_IN_ROOT_SIG, LIGHTS_BUFFER_POS_IN_ROOT_SIG, LIGHTS_CLUSTERS_BUFFER_POS_IN_ROOT_SIG, LIGHTS_CLUSTERS_DATA_BUFFER_POS_IN_ROOT_SIG, list, false);

			list->DrawIndexedInstanced(GetIndexSize(m_pScreen.get(), 0), 1, GetIndexStartPos(m_pScreen.get(), 0), GetVertexStartPos(m_pScreen.get(), 0), 0);


			m_pRenderableObjectsManager->ForwardRender(list);


			m_pForwardRenderPassRTV->EndDraw(list);
		}

		//
		///////////////////////////

		//////////////////////////
		// POSTPROCESS PASS

		BindMainViewPort(list);
		BindMainRect(list);
		BeginDraw(list);

		{

			list->ClearRenderTargetView(GetCurrBackBufferView(), COLOR, 0, nullptr);
			list->OMSetRenderTargets(1, &FD3DW::keep(GetCurrBackBufferView()), true, nullptr);

			PSOManager::GetInstance()->GetPSOObject(PSOType::PostProcessDefaultConfig)->Bind(list);

			list->IASetVertexBuffers(0, 1, m_pScreen->GetVertexBufferView());
			list->IASetIndexBuffer(m_pScreen->GetIndexBufferView());

			ID3D12DescriptorHeap* heaps[] = { m_pForwardRenderPassSRVPack->GetResult()->GetDescriptorPtr() };
			list->SetDescriptorHeaps(_countof(heaps), heaps);

			list->SetGraphicsRootDescriptorTable(0, m_pForwardRenderPassSRVPack->GetResult()->GetGPUDescriptorHandle(0));


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
	auto iii = GlobalRenderThreadManager::GetInstance()->Submit(gSecondPassRecipe, { gBufferH,shadowsH,clusterGenerationsH });

	auto presentH = GlobalRenderThreadManager::GetInstance()->SubmitLambda([this]() { PresentSwapchain(); }, { iii });

	auto afterSwap = GlobalRenderThreadManager::GetInstance()->SubmitLambda([this]() { 
		m_pRenderableObjectsManager->AfterRender();
		++m_uFrameIndex;
	}, { presentH });


	m_dInFlight.push_back(afterSwap);

	CallAfterRenderLoop();

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

	DestroyComponent(m_pLightsManager);
	DestroyComponent(m_pRenderableObjectsManager);
	DestroyComponent(m_pCameraComponent);
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

FD3DW::DepthStencilView* MainRenderer::GetDepthResource() {
	return m_pDSV.get();
}

bool MainRenderer::IsEnabledPreDepth() {
	return m_bIsEnabledPreDepth;
}

void MainRenderer::EnablePreDepth(bool in) {
	m_bIsEnabledPreDepth = in;
}

dx::XMMATRIX MainRenderer::GetCurrentProjectionMatrix() const {
	return m_pCameraComponent->GetProjectionMatrix();
}

dx::XMMATRIX MainRenderer::GetCurrentViewMatrix() const {
	return m_pCameraComponent->GetViewMatrix();
}

dx::XMFLOAT3 MainRenderer::GetCurrentCameraPosition() const {
	return m_pCameraComponent->GetCameraPosition();
}

float MainRenderer::GetCameraSpeed() {
	return m_pCameraComponent->GetCameraSpeed();
}

void MainRenderer::SetCameraSpeed(float speed) {
	m_pCameraComponent->SetCameraSpeed(speed);
}

void MainRenderer::SetDefaultPosition() {
	m_pCameraComponent->ResetPosition();
}

CameraFrustum MainRenderer::GetCameraFrustum() {
	return m_pCameraComponent->GetCameraFrustum();
}

std::vector<BaseRenderableObject*> MainRenderer::GetRenderableObjects() const {
	return m_pRenderableObjectsManager->GetRenderableObjects();
}

void MainRenderer::AddScene(std::string path) {
	ScheduleCreation([this, path](ID3D12GraphicsCommandList* list, ID3D12GraphicsCommandList4* dxrList) {
		m_pRenderableObjectsManager->CreateObject<RenderableMesh>(list, dxrList, path);
	});
}

void MainRenderer::AddSkybox(std::string path) {
	ScheduleCreation([this, path](ID3D12GraphicsCommandList* list, ID3D12GraphicsCommandList4* dxrList) {
		m_pRenderableObjectsManager->CreateObject<RenderableSkyboxObject>(list, dxrList, path);
	});
}

void MainRenderer::AddAudio(std::string path) {
	ScheduleCreation([this, path](ID3D12GraphicsCommandList* list, ID3D12GraphicsCommandList4* dxrList) {
		m_pRenderableObjectsManager->CreateObject<RenderableAudioObject>(list, dxrList, path);
	});
}

void MainRenderer::AddSimplePlane() {
	ScheduleCreation([this](ID3D12GraphicsCommandList* list, ID3D12GraphicsCommandList4* dxrList) {
		m_pRenderableObjectsManager->CreatePlane(list, dxrList);
	});
}

void MainRenderer::AddSimpleCone() {
	ScheduleCreation([this](ID3D12GraphicsCommandList* list, ID3D12GraphicsCommandList4* dxrList) {
		m_pRenderableObjectsManager->CreateCone(list, dxrList);
	});
}

void MainRenderer::AddSimpleCube() {
	ScheduleCreation([this](ID3D12GraphicsCommandList* list, ID3D12GraphicsCommandList4* dxrList) {
		m_pRenderableObjectsManager->CreateCube(list, dxrList);
	});
}

void MainRenderer::AddSimpleSphere() {
	ScheduleCreation([this](ID3D12GraphicsCommandList*list, ID3D12GraphicsCommandList4*dxrList) {
		m_pRenderableObjectsManager->CreateSphere(list, dxrList);
	});
}

void MainRenderer::RemoveObject(BaseRenderableObject* obj) {
	m_pRenderableObjectsManager->RemoveObject(obj);
}

void MainRenderer::RemoveAllObjects() {
	const auto& objs = GetRenderableObjects();
	for (const auto obj : objs) {
		RemoveObject(obj);
	}
}

void MainRenderer::SetMeshCullingType(CullingType in) {
	m_pRenderableObjectsManager->SetMeshCullingType(in);
}

CullingType MainRenderer::GetMeshCullingType() {
	return m_pRenderableObjectsManager->GetMeshCullingType();
}

FD3DW::AccelerationStructureBuffers MainRenderer::GetTLAS(ID3D12GraphicsCommandList4* list)
{
	return m_pRenderableObjectsManager->GetTLASData(GetDXRDevice(), list);
}

void MainRenderer::CreateLight() {
	LightStruct light;
	m_pLightsManager->AddLight(light);
}

const LightStruct& MainRenderer::GetLight(int idx) {
	return m_pLightsManager->GetLight(idx);
}

void MainRenderer::SetLightData(LightStruct newData, int idx) {
	m_pLightsManager->SetLightData(newData,idx);
}

int MainRenderer::GetLightsCount() {
	return m_pLightsManager->GetLightsCount();
}

void MainRenderer::BindLightConstantBuffer(UINT cbSlot, UINT rootSRVSlot, UINT rootSRVClustersSlot, UINT cbClusterDataSlot, ID3D12GraphicsCommandList* list, bool IsCompute) {
	m_pLightsManager->BindLightConstantBuffer(cbSlot, rootSRVSlot, rootSRVClustersSlot, cbClusterDataSlot, list, IsCompute);
}


ShadowType MainRenderer::CurrentShadowType() {
	return m_pShadowsComponent ? m_pShadowsComponent->Type() : ShadowType::None;
}

void MainRenderer::SetRTShadowConfig(RTSoftShadowConfig config) {
	if (auto rtSoftShadows = dynamic_cast<MainRenderer_RTSoftShadowsComponent*>(m_pShadowsComponent.get())) {
		rtSoftShadows->SetConfig(config);
	}
}

RTSoftShadowConfig MainRenderer::GetRTShadowConfig() {
	if (auto rtSoftShadows = dynamic_cast<MainRenderer_RTSoftShadowsComponent*>(m_pShadowsComponent.get())) {
		return rtSoftShadows->GetConfig();
	}
	return {};
}

void MainRenderer::SaveSceneToFile(std::string pathTo) {
	AddToCallAfterRenderLoop([this, pathTo]() {
		BinarySerializer ser;
		ser.LoadFromObjects(m_pCameraComponent, m_pLightsManager, m_pRenderableObjectsManager, m_pShadowsComponent);

		ser.SaveToFile(pathTo);
	});
}

void MainRenderer::LoadSceneFromFile(std::string pathTo) {
	AddToCallAfterRenderLoop([this, pathTo]() {
		BinarySerializer ser;
		ser.LoadFromFile(pathTo);

		this->DestroyComponent(m_pCameraComponent);
		this->DestroyComponent(m_pLightsManager);
		this->DestroyComponent(m_pRenderableObjectsManager);
		if(m_pShadowsComponent) this->DestroyComponent(m_pShadowsComponent);

		ser.DeserializeToObjects(m_pCameraComponent, m_pLightsManager, m_pRenderableObjectsManager, m_pShadowsComponent);
		m_pCameraComponent->SetAfterConstruction(this);
		m_pLightsManager->SetAfterConstruction(this);
		
		auto ltcRecipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_DIRECT, [this](ID3D12GraphicsCommandList* list) {
			m_pLightsManager->InitLTC(list, m_pGBuffersSRVPack.get());
		});
		auto ltcH = GlobalRenderThreadManager::GetInstance()->Submit(ltcRecipe);
		ltcH->WaitForExecute();

		m_pRenderableObjectsManager->SetAfterConstruction(this);

		if (!m_pShadowsComponent) 
		{
			this->TryInitShadowComponent();

		} 
		else if (!m_pShadowsComponent->IsCanBeEnabled(this))
		{
			this->DestroyComponent(m_pShadowsComponent);
			this->TryInitShadowComponent();
		}
		else 
		{
			m_pShadowsComponent->SetAfterConstruction(this);
			CustomAfterInitShadowComponent(m_pShadowsComponent.get());
		}
	});
}

void MainRenderer::DeleteLight(int idx) {
	m_pLightsManager->DeleteLight(idx);
}

void MainRenderer::InitMainRendererComponents()
{
	m_pUIComponent = CreateUniqueComponent<MainRenderer_UIComponent>();
	m_pCameraComponent = CreateUniqueComponent<MainRenderer_CameraComponent>();
	m_pRenderableObjectsManager = CreateUniqueComponent<MainRenderer_RenderableObjectsManager>();
	m_pLightsManager = CreateUniqueComponent<MainRenderer_LightsManager>();
}

void MainRenderer::InitMainRendererParts(ID3D12Device* device) {
	InitializeDescriptorSizes(device, Get_RTV_DescriptorSize(), Get_DSV_DescriptorSize(), Get_CBV_SRV_UAV_DescriptorSize());
	PSOManager::GetInstance()->InitPSOjects(device);
	BaseRenderableObject::CreateEmptyStructuredBuffer(device);
	GlobalTextureHeap::GetInstance()->Init(device, GLOBAL_TEXTURE_HEAP_PRECACHE_SIZE,GLOBAL_TEXTURE_HEAP_NODE_MASK);
	
}

void MainRenderer::InitMainRendererDXRParts(ID3D12Device5* device)
{
	PSOManager::GetInstance()->InitPSOjectsDevice5(device);
}

void MainRenderer::TryInitShadowComponent() {
	if (IsRTSupported()) {
		auto rtSoftShadow = CreateUniqueComponent<MainRenderer_RTSoftShadowsComponent>();
		CustomAfterInitShadowComponent(rtSoftShadow.get());
		m_pShadowsComponent = std::move(rtSoftShadow);
	}
}

void MainRenderer::CustomAfterInitShadowComponent(MainRenderer_ShadowsComponent* shadow) {
	auto device = GetDevice();
	if (shadow) {
		shadow->BindResultResource(device, m_pGBuffersSRVPack.get(), SHADOW_FACTOR_LOCATION_IN_HEAP);

	}
	else {
		m_pGBuffersSRVPack->AddNullResource(SHADOW_FACTOR_LOCATION_IN_HEAP, device);
	}

	if (auto rtShadow = dynamic_cast<MainRenderer_RTSoftShadowsComponent*>(shadow)) {
		rtShadow->SetGBuffersResources(m_pGBuffers[0]->GetTexture(), m_pGBuffers[1]->GetTexture(), device);
	}
}

void MainRenderer::AddToCallAfterRenderLoop(std::function<void(void)> foo) {
	m_vCallAfterRenderLoop.push_back(foo);
}

void MainRenderer::CallAfterRenderLoop() {
	if (m_vCallAfterRenderLoop.empty()) return;

	GlobalRenderThreadManager::GetInstance()->WaitIdle();

	auto vv = m_vCallAfterRenderLoop;
	m_vCallAfterRenderLoop.clear();
	for (auto han : vv) {
		if (han) han();
	}

	GlobalRenderThreadManager::GetInstance()->WaitIdle();

}


