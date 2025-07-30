#include <MainRenderer/MainRenderer.h>
#include <MainRenderer/PSOManager.h>
#include <D3DFramework/Utilites/Serializer/BinarySerializer.h>
#include <D3DFramework/GraphicUtilites/CommandList.h>

static FLOAT COLOR[4] = { 0.2f,0.2f,0.2f,1.0f };

MainRenderer::MainRenderer() : WinWindow(L"FDW", 1024, 1024, false) {}

void MainRenderer::UserInit()
{
	auto device = GetDevice();

	InitMainRendererParts(device);

	SetVSync(true);

	m_pDSV = CreateDepthStencilView(DXGI_FORMAT_D24_UNORM_S8_UINT, D3D12_DSV_DIMENSION_TEXTURE2D, 1, 1024, 1024);
	m_pDSVPack = CreateDSVPack(1u);
	m_pDSVPack->PushResource(m_pDSV->GetDSVResource(), m_pDSV->GetDSVDesc(), device);

	auto wndSettins = GetMainWNDSettings();

	const auto& gBufferFormats = GetGBufferData().GBuffersFormats;
	auto gbuffersNum = (UINT)gBufferFormats.size();
	m_pGBuffersRTVPack = CreateRTVPack(gbuffersNum);
	m_pGBuffersSRVPack = CreateSRVPack(COUNT_SRV_IN_GBUFFER_HEAP);

	for (const auto& format : gBufferFormats) {
		m_pGBuffers.push_back(CreateRenderTarget(format, D3D12_RTV_DIMENSION_TEXTURE2D, 1, wndSettins.Width, wndSettins.Height));
		auto& gbuffer = m_pGBuffers.back();
		
		m_pGBuffersRTVPack->PushResource(gbuffer->GetRTVResource(), gbuffer->GetRTVDesc(), device);
		m_pGBuffersSRVPack->PushResource(gbuffer->GetRTVResource(), D3D12_SRV_DIMENSION_TEXTURE2D, device);
	}

	m_pLightsManager->InitLTC(m_pPCML, m_pGBuffersSRVPack.get());

	m_pForwardRenderPassRTV = CreateRenderTarget(GetForwardRenderPassFormat(), D3D12_RTV_DIMENSION_TEXTURE2D, 1, wndSettins.Width, wndSettins.Height);
	m_pForwardRenderPassRTVPack = CreateRTVPack(1u);
	m_pForwardRenderPassSRVPack = CreateSRVPack(1u);
	m_pForwardRenderPassRTVPack->PushResource(m_pForwardRenderPassRTV->GetRTVResource(), m_pForwardRenderPassRTV->GetRTVDesc(), device);
	m_pForwardRenderPassSRVPack->PushResource(m_pForwardRenderPassRTV->GetRTVResource(), D3D12_SRV_DIMENSION_TEXTURE2D, device);


	m_pScreen = CreateRectangle(m_pPCML);

	m_xSceneViewPort.MaxDepth = 1.0f;
	m_xSceneViewPort.MinDepth = 0.0f;
	m_xSceneViewPort.Height = (float)wndSettins.Height;
	m_xSceneViewPort.Width = (float)wndSettins.Width;
	m_xSceneViewPort.TopLeftX = 0;
	m_xSceneViewPort.TopLeftY = 0;

	m_xSceneRect.left = 0;
	m_xSceneRect.right = wndSettins.Width;
	m_xSceneRect.top = 0;
	m_xSceneRect.bottom = wndSettins.Height;

	ExecuteMainQueue();
	FD3DW::FResource::ReleaseUploadBuffers();
}

void MainRenderer::UserLoop()
{
	m_pCommandList->ResetList();

	m_pLightsManager->BeforeRender(m_pPCML);
	m_pRenderableObjectsManager->BeforeRender(m_pPCML);

	m_pPCML->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	
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

		m_pLightsManager->BindLightConstantBuffer(LIGHTS_HELPER_BUFFER_POS_IN_ROOT_SIG, LIGHTS_BUFFER_POS_IN_ROOT_SIG, m_pPCML);

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
	m_pRenderableObjectsManager->CreateObject<RenderableMesh>(GetDevice(), m_pPCML, path);
}

void MainRenderer::AddSkybox(std::string path) {
	m_pRenderableObjectsManager->CreateObject<RenderableSkyboxObject>(GetDevice(), m_pPCML, path);
}

void MainRenderer::AddAudio(std::string path) {
	m_pRenderableObjectsManager->CreateObject<RenderableAudioObject>(GetDevice(), m_pPCML, path);
}

void MainRenderer::AddSimplePlane() {
	m_pRenderableObjectsManager->CreatePlane(m_pPCML);
}

void MainRenderer::AddSimpleCone() {
	m_pRenderableObjectsManager->CreateCone(m_pPCML);
}

void MainRenderer::AddSimpleCube() {
	m_pRenderableObjectsManager->CreateCube(m_pPCML);
}

void MainRenderer::AddSimpleSphere() {
	m_pRenderableObjectsManager->CreateSphere(m_pPCML);
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

void MainRenderer::SaveSceneToFile(std::string pathTo) {
	AddToCallAfterRenderLoop([this, pathTo]() {
		BinarySerializer ser;
		ser.LoadFromObjects(m_pCameraComponent, m_pLightsManager, m_pRenderableObjectsManager);

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

		ser.DeserializeToObjects(m_pCameraComponent, m_pLightsManager, m_pRenderableObjectsManager);
		m_pCameraComponent->SetAfterConstruction(this);
		m_pLightsManager->SetAfterConstruction(this);
		m_pLightsManager->InitLTC(m_pPCML, m_pGBuffersSRVPack.get());
		m_pRenderableObjectsManager->SetAfterConstruction(this);
	});
}

void MainRenderer::DeleteLight(int idx) {
	m_pLightsManager->DeleteLight(idx);
}

void MainRenderer::InitMainRendererParts(ID3D12Device* device) {
	InitializeDescriptorSizes(device, Get_RTV_DescriptorSize(), Get_DSV_DescriptorSize(), Get_CBV_SRV_UAV_DescriptorSize());
	PSOManager::GetInstance()->InitPSOjects(device);
	BaseRenderableObject::CreateEmptyStructuredBuffer(device);

	m_pCommandList = CreateList(D3D12_COMMAND_LIST_TYPE_DIRECT);
	m_pPCML = m_pCommandList->GetPtrCommandList();
	BindMainCommandList(m_pCommandList.get());

	m_pUIComponent = CreateUniqueComponent<MainRenderer_UIComponent>();
	m_pCameraComponent = CreateUniqueComponent<MainRenderer_CameraComponent>();
	m_pRenderableObjectsManager = CreateUniqueComponent<MainRenderer_RenderableObjectsManager>();
	m_pLightsManager = CreateUniqueComponent<MainRenderer_LightsManager>();

}

void MainRenderer::AddToCallAfterRenderLoop(std::function<void(void)> foo) {
	m_vCallAfterRenderLoop.push_back(foo);
}

void MainRenderer::CallAfterRenderLoop() {
	m_pCommandList->ResetList();

	auto vv = m_vCallAfterRenderLoop;
	m_vCallAfterRenderLoop.clear();
	for (auto han : vv) {
		if (han) han();
	}

	ExecuteMainQueue();
}


