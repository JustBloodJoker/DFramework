#include <MainRenderer/MainRenderer.h>
#include <MainRenderer/PSOManager.h>

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
	m_pRTV = CreateRenderTarget(DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RTV_DIMENSION_TEXTURE2D, 1, wndSettins.Width, wndSettins.Height);
	m_pRTVPack = CreateRTVPack(1u);
	m_pRTVPack->PushResource(m_pRTV->GetRTVResource(), m_pRTV->GetRTVDesc(), device);
	m_pRTV_SRVPack = CreateSRVPack(1u);
	m_pRTV_SRVPack->PushResource(m_pRTV->GetRTVResource(), D3D12_SRV_DIMENSION_TEXTURE2D, device);

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

	m_pRenderableObjectsManager->BeforeRender(GetDevice(), m_pPCML);

	m_pPCML->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	
	///////////////////////
	//	DEFERRED FIRST PASS
	m_pRTV->StartDraw(m_pPCML);
	
	m_pPCML->RSSetScissorRects(1, &m_xSceneRect);
	m_pPCML->RSSetViewports(1, &m_xSceneViewPort);

	m_pPCML->ClearDepthStencilView(m_pDSVPack->GetResult()->GetCPUDescriptorHandle(0), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	m_pPCML->ClearRenderTargetView(m_pRTVPack->GetResult()->GetCPUDescriptorHandle(0), COLOR, 0, nullptr);
	m_pPCML->OMSetRenderTargets(1, &FD3DW::keep(m_pRTVPack->GetResult()->GetCPUDescriptorHandle(0)), true, &FD3DW::keep(m_pDSVPack->GetResult()->GetCPUDescriptorHandle(0)));

	m_pRenderableObjectsManager->DeferredRender(m_pPCML);

	m_pRTV->EndDraw(m_pPCML);

	///
	///////////////////////////

	//////////////////////////
	// DEFERRED SECOND PASS

	BindMainViewPort(m_pPCML);
	BindMainRect(m_pPCML);
	BeginDraw(m_pCommandList->GetPtrCommandList());

	m_pPCML->ClearRenderTargetView(GetCurrBackBufferView(), COLOR, 0, nullptr);
	m_pPCML->OMSetRenderTargets(1, &FD3DW::keep(GetCurrBackBufferView()), true, &FD3DW::keep(m_pDSVPack->GetResult()->GetCPUDescriptorHandle(0)));
	
	PSOManager::GetInstance()->GetPSOObject(PSOType::DefferedSecondPassDefaultConfig)->Bind(m_pPCML);

	m_pPCML->IASetVertexBuffers(0, 1, m_pScreen->GetVertexBufferView());
	m_pPCML->IASetIndexBuffer(m_pScreen->GetIndexBufferView());

	ID3D12DescriptorHeap* heaps[] = { m_pRTV_SRVPack->GetResult()->GetDescriptorPtr() };
	m_pPCML->SetDescriptorHeaps(_countof(heaps), heaps);

	m_pPCML->SetGraphicsRootDescriptorTable(0, m_pRTV_SRVPack->GetResult()->GetGPUDescriptorHandle(0));

	m_pPCML->DrawIndexedInstanced(GetIndexSize(m_pScreen.get(), 0), 1, GetIndexStartPos(m_pScreen.get(), 0), GetVertexStartPos(m_pScreen.get(), 0), 0);

	///
	///////////////////////////
	

	//////////////////////////
	//			FORWARD PASS

	m_pPCML->OMSetRenderTargets(1, &FD3DW::keep(GetCurrBackBufferView()), true, &FD3DW::keep(m_pDSVPack->GetResult()->GetCPUDescriptorHandle(0)));

	m_pRenderableObjectsManager->ForwardRender(m_pPCML);

	m_pUIComponent->RenderImGui();
	
	///
	///////////////////////////

	EndDraw(m_pCommandList->GetPtrCommandList());

	ExecuteMainQueue();
	
	m_pRenderableObjectsManager->AfterRender();
}

void MainRenderer::UserClose()
{
	while ( !m_vComponents.empty() ) DestroyComponent( m_vComponents.back().get() );

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

std::vector<BaseRenderableObject*> MainRenderer::GetRenderableObjects() const {
	return m_pRenderableObjectsManager->GetRenderableObjects();
}

void MainRenderer::AddScene(std::string path) {
	m_pRenderableObjectsManager->CreateObject(CreateScene(path, true, m_pPCML), GetDevice(), m_pPCML);
}

void MainRenderer::AddSkybox(std::string path) {
	m_pRenderableObjectsManager->CreateObject(path, GetDevice(), m_pPCML);
}

void MainRenderer::AddAudio(std::string path) {
	m_pRenderableObjectsManager->CreateObject(CreateAudio(FD3DW::StringToWString(path)), GetDevice(), m_pPCML, path);
}

void MainRenderer::AddSimplePlane() {
	m_pRenderableObjectsManager->CreatePlane(GetDevice(), m_pPCML);
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

void MainRenderer::InitMainRendererParts(ID3D12Device* device) {
	InitializeDescriptorSizes(device, Get_RTV_DescriptorSize(), Get_DSV_DescriptorSize(), Get_CBV_SRV_UAV_DescriptorSize());
	PSOManager::GetInstance()->InitPSOjects(device);
	BaseRenderableObject::CreateEmptyStructuredBuffer(device);

	m_pCommandList = CreateList(D3D12_COMMAND_LIST_TYPE_DIRECT);
	m_pPCML = m_pCommandList->GetPtrCommandList();
	BindMainCommandList(m_pCommandList.get());

	m_pUIComponent = CreateComponent<MainRenderer_UIComponent>();
	m_pCameraComponent = CreateComponent<MainRenderer_CameraComponent>();
	m_pRenderableObjectsManager = CreateComponent<MainRenderer_RenderableObjectsManager>();

}

void MainRenderer::DestroyComponent(MainRendererComponent* cmp) {
	auto it = std::find_if(m_vComponents.begin(), m_vComponents.end(),
		[cmp](const std::unique_ptr<MainRendererComponent>& ptr) {
			return ptr.get() == cmp;
		});

	if (it != m_vComponents.end()) {
		(*it)->BeforeDestruction();
		m_vComponents.erase(it);
	}
}
