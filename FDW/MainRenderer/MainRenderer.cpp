#include <MainRenderer/MainRenderer.h>
#include <MainRenderer/PSOManager.h>
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

	m_pDSV = CreateDepthStencilView(DXGI_FORMAT_D24_UNORM_S8_UINT, D3D12_DSV_DIMENSION_TEXTURE2D, 1, 1024, 1024);
	m_pDSVPack = CreateDSVPack(1u);
	m_pDSVPack->PushResource(m_pDSV->GetDSVResource(), m_pDSV->GetDSVDesc(), device);

	auto wndSettings = GetMainWNDSettings();

	const auto& gBufferFormats = GetGBufferData().GBuffersFormats;
	auto gbuffersNum = (UINT)gBufferFormats.size();
	m_pGBuffersRTVPack = CreateRTVPack(gbuffersNum);
	m_pGBuffersSRVPack = CreateSRVPack(COUNT_SRV_IN_GBUFFER_HEAP);

	for (const auto& format : gBufferFormats) {
		m_pGBuffers.push_back(CreateRenderTarget(format, D3D12_RTV_DIMENSION_TEXTURE2D, 1, wndSettings.Width, wndSettings.Height));
		auto& gbuffer = m_pGBuffers.back();
		
		m_pGBuffersRTVPack->PushResource(gbuffer->GetRTVResource(), gbuffer->GetRTVDesc(), device);
		m_pGBuffersSRVPack->PushResource(gbuffer->GetRTVResource(), D3D12_SRV_DIMENSION_TEXTURE2D, device);
	}

	m_pLightsManager->InitLTC(m_pPCML, m_pGBuffersSRVPack.get());

	m_pForwardRenderPassRTV = CreateRenderTarget(GetForwardRenderPassFormat(), D3D12_RTV_DIMENSION_TEXTURE2D, 1, wndSettings.Width, wndSettings.Height);
	m_pForwardRenderPassRTVPack = CreateRTVPack(1u);
	m_pForwardRenderPassSRVPack = CreateSRVPack(1u);
	m_pForwardRenderPassRTVPack->PushResource(m_pForwardRenderPassRTV->GetRTVResource(), m_pForwardRenderPassRTV->GetRTVDesc(), device);
	m_pForwardRenderPassSRVPack->PushResource(m_pForwardRenderPassRTV->GetRTVResource(), D3D12_SRV_DIMENSION_TEXTURE2D, device);

	m_pScreen = CreateRectangle(m_pPCML);
	
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

	ExecuteMainQueue();
	if(m_pDXRCommandQueue) m_pDXRCommandQueue->ExecuteQueue(true);
	FD3DW::FResource::ReleaseUploadBuffers();
}

void MainRenderer::UserLoop()
{
	m_pCommandList->ResetList();
	if (m_pDXRCommandList) m_pDXRCommandList->ResetList();

	m_pLightsManager->BeforeRender(m_pPCML);
	m_pRenderableObjectsManager->BeforeRender(m_pPCML);
	if (m_pShadowsComponent) m_pShadowsComponent->BeforeRender(m_pPCML);


	m_pPCML->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	
	///////////////////////
	//	Shadow PASS Before gbuffer pass
	{
		if (m_pShadowsComponent) {
			m_pShadowsComponent->BeforeGBufferPass();
		}
	}
	///
	///////////////////////////


	///////////////////////
	//	DEFERRED FIRST PASS
	{
		auto gBuffersCount = GetGBuffersNum();

		for (auto& gbuffer : m_pGBuffers) {
			gbuffer->StartDraw(m_pPCML);
		}

		m_pPCML->RSSetScissorRects(1, &m_xSceneRect);
		m_pPCML->RSSetViewports(1, &m_xSceneViewPort);

		m_pPCML->ClearDepthStencilView(m_pDSVPack->GetResult()->GetCPUDescriptorHandle(0), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
		for (UINT i = 0; i < gBuffersCount; ++i) {
			m_pPCML->ClearRenderTargetView(m_pGBuffersRTVPack->GetResult()->GetCPUDescriptorHandle(i), COLOR, 0, nullptr);
		}
		m_pPCML->OMSetRenderTargets(gBuffersCount, &FD3DW::keep(m_pGBuffersRTVPack->GetResult()->GetCPUDescriptorHandle(0)), true, &FD3DW::keep(m_pDSVPack->GetResult()->GetCPUDescriptorHandle(0)));
		
		m_pRenderableObjectsManager->DeferredRender(m_pPCML);

		for (auto& gbuffer : m_pGBuffers) {
			gbuffer->EndDraw(m_pPCML);
		}

	}

	///
	///////////////////////////

	///////////////////////
	//	Shadow PASS After GBuffer pass
	{
		if (m_pShadowsComponent) {
			ExecuteMainQueue();
			m_pCommandList->ResetList();
			m_pPCML->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			m_pShadowsComponent->AfterGBufferPass();
		}
	}
	///
	///////////////////////////

	//////////////////////////
	// DEFERRED SECOND PASS

	{
		m_pForwardRenderPassRTV->StartDraw(m_pPCML);

		m_pPCML->RSSetScissorRects(1, &m_xSceneRect);
		m_pPCML->RSSetViewports(1, &m_xSceneViewPort);

		m_pPCML->ClearRenderTargetView(m_pForwardRenderPassRTVPack->GetResult()->GetCPUDescriptorHandle(0), COLOR, 0, nullptr);
		m_pPCML->OMSetRenderTargets(1, &FD3DW::keep(m_pForwardRenderPassRTVPack->GetResult()->GetCPUDescriptorHandle(0)), true, &FD3DW::keep(m_pDSVPack->GetResult()->GetCPUDescriptorHandle(0)));

		PSOManager::GetInstance()->GetPSOObject(PSOType::DefferedSecondPassDefaultConfig)->Bind(m_pPCML);

		m_pPCML->IASetVertexBuffers(0, 1, m_pScreen->GetVertexBufferView());
		m_pPCML->IASetIndexBuffer(m_pScreen->GetIndexBufferView());

		ID3D12DescriptorHeap* heaps[] = { m_pGBuffersSRVPack->GetResult()->GetDescriptorPtr() };
		m_pPCML->SetDescriptorHeaps(_countof(heaps), heaps);

		m_pPCML->SetGraphicsRootDescriptorTable(DEFFERED_GBUFFERS_POS_IN_ROOT_SIG, m_pGBuffersSRVPack->GetResult()->GetGPUDescriptorHandle(0));

		m_pLightsManager->BindLightConstantBuffer(LIGHTS_HELPER_BUFFER_POS_IN_ROOT_SIG, LIGHTS_BUFFER_POS_IN_ROOT_SIG, m_pPCML, false);

		m_pPCML->DrawIndexedInstanced(GetIndexSize(m_pScreen.get(), 0), 1, GetIndexStartPos(m_pScreen.get(), 0), GetVertexStartPos(m_pScreen.get(), 0), 0);


		m_pRenderableObjectsManager->ForwardRender(m_pPCML);


		m_pForwardRenderPassRTV->EndDraw(m_pPCML);
	}

	//
	///////////////////////////

	//////////////////////////
	// POSTPROCESS PASS

	BindMainViewPort(m_pPCML);
	BindMainRect(m_pPCML);
	BeginDraw(m_pCommandList->GetPtrCommandList());

	{

		m_pPCML->ClearRenderTargetView(GetCurrBackBufferView(), COLOR, 0, nullptr);
		m_pPCML->OMSetRenderTargets(1, &FD3DW::keep(GetCurrBackBufferView()), true, nullptr);

		PSOManager::GetInstance()->GetPSOObject(PSOType::PostProcessDefaultConfig)->Bind(m_pPCML);

		m_pPCML->IASetVertexBuffers(0, 1, m_pScreen->GetVertexBufferView());
		m_pPCML->IASetIndexBuffer(m_pScreen->GetIndexBufferView());

		ID3D12DescriptorHeap* heaps[] = { m_pForwardRenderPassSRVPack->GetResult()->GetDescriptorPtr() };
		m_pPCML->SetDescriptorHeaps(_countof(heaps), heaps);

		m_pPCML->SetGraphicsRootDescriptorTable(0, m_pForwardRenderPassSRVPack->GetResult()->GetGPUDescriptorHandle(0));


		m_pPCML->DrawIndexedInstanced(GetIndexSize(m_pScreen.get(), 0), 1, GetIndexStartPos(m_pScreen.get(), 0), GetVertexStartPos(m_pScreen.get(), 0), 0);

	}

	///
	///////////////////////////
	

	//////////////////////////
	//			UI PASS

	{
		m_pUIComponent->RenderImGui();
	}
	
	///
	///////////////////////////

	{
		EndDraw(m_pCommandList->GetPtrCommandList());
	}

	if(m_pDXRCommandQueue)m_pDXRCommandQueue->ExecuteQueue(true);
	ExecuteMainQueue();			

	m_pRenderableObjectsManager->AfterRender();

	CallAfterRenderLoop();
}

void MainRenderer::UserClose()
{
	DestroyComponent(m_pLightsManager);
	DestroyComponent(m_pRenderableObjectsManager);
	DestroyComponent(m_pCameraComponent);
	DestroyComponent(m_pUIComponent);

	PSOManager::FreeInstance();
}

ID3D12GraphicsCommandList4* MainRenderer::GetDXRCommandList()
{
	return m_pDXRPCML;
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


std::vector<BaseRenderableObject*> MainRenderer::GetRenderableObjects() const {
	return m_pRenderableObjectsManager->GetRenderableObjects();
}

void MainRenderer::AddScene(std::string path) {
	ScheduleCreation([this, path]() {
		m_pRenderableObjectsManager->CreateObject<RenderableMesh>(m_pPCML, m_pDXRPCML, path);
	});
}

void MainRenderer::AddSkybox(std::string path) {
	ScheduleCreation([this, path]() {
		m_pRenderableObjectsManager->CreateObject<RenderableSkyboxObject>(m_pPCML, m_pDXRPCML, path);
	});
}

void MainRenderer::AddAudio(std::string path) {
	ScheduleCreation([this, path]() {
		m_pRenderableObjectsManager->CreateObject<RenderableAudioObject>(m_pPCML, m_pDXRPCML, path);
	});
}

void MainRenderer::AddSimplePlane() {
	ScheduleCreation([this]() {
		m_pRenderableObjectsManager->CreatePlane(m_pPCML, m_pDXRPCML);
	});
}

void MainRenderer::AddSimpleCone() {
	ScheduleCreation([this]() {
		m_pRenderableObjectsManager->CreateCone(m_pPCML, m_pDXRPCML);
	});
}

void MainRenderer::AddSimpleCube() {
	ScheduleCreation([this]() {
		m_pRenderableObjectsManager->CreateCube(m_pPCML, m_pDXRPCML);
	});
}

void MainRenderer::AddSimpleSphere() {
	ScheduleCreation([this]() {
		m_pRenderableObjectsManager->CreateSphere(m_pPCML, m_pDXRPCML);
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

void MainRenderer::BindLightConstantBuffer(UINT cbSlot, UINT rootSRVSlot, ID3D12GraphicsCommandList* list, bool IsCompute) {
	m_pLightsManager->BindLightConstantBuffer(cbSlot, rootSRVSlot, list, IsCompute);
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
		m_pLightsManager->InitLTC(m_pPCML, m_pGBuffersSRVPack.get());
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

	m_pCommandList = CreateList(D3D12_COMMAND_LIST_TYPE_DIRECT);
	m_pPCML = m_pCommandList->GetPtrCommandList();
	BindMainCommandList(m_pCommandList.get());
}

void MainRenderer::InitMainRendererDXRParts(ID3D12Device5* device)
{
	PSOManager::GetInstance()->InitPSOjectsDevice5(device);

	m_pDXRCommandList = FD3DW::DXRCommandList::CreateList(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
	m_pDXRPCML = m_pDXRCommandList->GetPtrCommandList();
	m_pDXRCommandQueue = FD3DW::CommandQueue::CreateQueue(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
	m_pDXRCommandQueue->BindCommandList(m_pDXRCommandList.get());
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
	m_pCommandList->ResetList();
	if (m_pDXRCommandList) m_pDXRCommandList->ResetList();

	auto vv = m_vCallAfterRenderLoop;
	m_vCallAfterRenderLoop.clear();
	for (auto han : vv) {
		if (han) han();
	}

	ExecuteMainQueue();
	if (m_pDXRCommandQueue) m_pDXRCommandQueue->ExecuteQueue(true);
}


