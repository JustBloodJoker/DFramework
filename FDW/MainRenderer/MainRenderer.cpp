#include <MainRenderer/MainRenderer.h>

static FLOAT COLOR[4] = { 0.2f,0.2f,0.2f,1.0f };

MainRenderer::MainRenderer() : WinWindow(L"FDW", 1024, 1024, false) {}

void MainRenderer::UserInit()
{
	SetVSync(true);

	m_pCommandList = CreateList(D3D12_COMMAND_LIST_TYPE_DIRECT);
	m_pPCML = m_pCommandList->GetPtrCommandList();

	m_pTimer = GetTimer();
	auto device = GetDevice();

	m_pUIComponent = CreateComponent<MainRenderer_UIComponent>();
	m_pCameraComponent = CreateComponent<MainRenderer_CameraComponent>();

	m_pMusic = CreateAudio(L"Content/322.wav");
	m_pMusic->SetVolume(0.1f);

	BindMainCommandList(m_pCommandList.get());

	m_pBird = CreateScene("Content/sampleModels/bird/scene.gltf", true, m_pPCML);

	for (size_t ind = 0; ind < m_pBird->GetMaterialSize(); ind++)
	{
		m_vSRVPacks.push_back(CreateSRVPack(1u));
		if (m_pBird->GetMaterialMananger()->GetMaterial(ind)->IsHaveTexture(TextureType::BASE))
		{
			(*m_vSRVPacks.rbegin())->PushResource(m_pBird->GetMaterialMananger()->GetMaterial(ind)->GetResourceTexture(TextureType::BASE), D3D12_SRV_DIMENSION_TEXTURE2D, device);
		}
	}
	m_pSamplerPack = CreateSamplerPack(1u);
	m_pSamplerPack->PushDefaultSampler(device);

	m_pDSV = CreateDepthStencilView(DXGI_FORMAT_D24_UNORM_S8_UINT, D3D12_DSV_DIMENSION_TEXTURE2D, 1, 1024, 1024);
	m_pDSVPack = CreateDSVPack(1u);
	m_pDSVPack->PushResource(m_pDSV->GetDSVResource(), m_pDSV->GetDSVDesc(), device);

	m_pStructureBufferBones = CreateSimpleStructuredBuffer(m_pBird->GetBonesCount() * sizeof(dx::XMMATRIX));

	m_pMatricesBuffer = CreateConstantBuffer<FD3DW::MatricesConstantBufferStructureFrameWork>(1);

	auto wndSettins = GetMainWNDSettings();
	m_pRTV = CreateRenderTarget(DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RTV_DIMENSION_TEXTURE2D, 1, wndSettins.Width, wndSettins.Height);
	m_pRTVPack = CreateRTVPack(1u);
	m_pRTVPack->PushResource(m_pRTV->GetRTVResource(), m_pRTV->GetRTVDesc(), device);
	m_pRTV_SRVPack = CreateSRVPack(1u);
	m_pRTV_SRVPack->PushResource(m_pRTV->GetRTVResource(), D3D12_SRV_DIMENSION_TEXTURE2D, device);

	CD3DX12_RASTERIZER_DESC rasterizerDesc(D3D12_DEFAULT);
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
	rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;

	FD3DW::GraphicPipelineObjectDesc config;
	config.RasterizerState = rasterizerDesc;
	config.RTVFormats[0] = m_pRTV->GetRTVDesc().Format;
	config.NumRenderTargets = 1;
	config.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	m_pPSODefferedFirstPass = std::make_unique<FD3DW::PipelineObject>(device);
	m_pPSODefferedFirstPass->SetIncludeDirectories({ L"Content/Shaders/DefaultInclude", L"Content/Shaders/DefferedFirstPassAnimatedMeshes" });
	m_pPSODefferedFirstPass->CreatePSO(
		{
			{ FD3DW::CompileFileType::VS, { L"Content/Shaders/DefferedFirstPassAnimatedMeshes/VS.hlsl", L"VS", L"vs_6_5" } },
			{ FD3DW::CompileFileType::PS, { L"Content/Shaders/DefferedFirstPassAnimatedMeshes/PS.hlsl", L"PS", L"ps_6_5" } },
			{ FD3DW::CompileFileType::RootSignature, { L"Content/Shaders/DefferedFirstPassAnimatedMeshes/RS.hlsl", L"RS", L"rootsig_1_1" } }
		},
		config
	);

	//auto staticPSO = std::make_unique<FD3DW::PipelineObject>(device);
	//staticPSO->SetIncludeDirectories({ L"Content/Shaders/DefaultInclude", L"Content/Shaders/DefferedFirstPassSimpleMeshes" });
	//staticPSO->CreatePSO(
	//	{
	//		{ FD3DW::CompileFileType::VS, { L"Content/Shaders/DefferedFirstPassSimpleMeshes/VS.hlsl", L"VS", L"vs_6_5" } },
	//		{ FD3DW::CompileFileType::PS, { L"Content/Shaders/DefferedFirstPassSimpleMeshes/PS.hlsl", L"PS", L"ps_6_5" } },
	//		{ FD3DW::CompileFileType::RootSignature, { L"Content/Shaders/DefferedFirstPassSimpleMeshes/RS.hlsl", L"RS", L"rootsig_1_1" } }
	//	},
	//	config
	//);

	FD3DW::GraphicPipelineObjectDesc config1;
	config1.RasterizerState = rasterizerDesc;
	config1.RTVFormats[0] = GetMainRTVFormat();
	config1.NumRenderTargets = 1;
	config1.DSVFormat = DXGI_FORMAT_UNKNOWN;
	config1.DepthStencilState = {};

	m_pPSODefferedSecondPass = std::make_unique<FD3DW::PipelineObject>(device);
	m_pPSODefferedSecondPass->SetIncludeDirectories({ L"Content/Shaders/DefaultInclude", L"Content/Shaders/DefferedSecondPass" });
	m_pPSODefferedSecondPass->CreatePSO(
		{
			{ FD3DW::CompileFileType::VS, { L"Content/Shaders/DefferedSecondPass/VS.hlsl", L"VS", L"vs_6_5" } },
			{ FD3DW::CompileFileType::PS, { L"Content/Shaders/DefferedSecondPass/PS.hlsl", L"PS", L"ps_6_5" } },
			{ FD3DW::CompileFileType::RootSignature, { L"Content/Shaders/DefferedSecondPass/RS.hlsl", L"RS", L"rootsig_1_1" } }
		},
		config1
	);
	
	m_pScreen = CreateRectangle(true, m_pPCML);

	m_xSceneViewPort.MaxDepth = 1.0f;
	m_xSceneViewPort.MinDepth = 0.0f;
	m_xSceneViewPort.Height = wndSettins.Height;
	m_xSceneViewPort.Width = wndSettins.Width;
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

	auto resultVector = m_pBird->PlayAnimation(m_pTimer->GetTime(), "Take 001");
	m_pStructureBufferBones->UploadData(GetDevice(), m_pPCML, resultVector.data(), false, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	FD3DW::MatricesConstantBufferStructureFrameWork cmb;
	cmb.Projection = dx::XMMatrixTranspose(m_pCameraComponent->GetProjectionMatrix());
	cmb.View = dx::XMMatrixTranspose(m_pCameraComponent->GetViewMatrix());
	m_xWorld = dx::XMMatrixIdentity();
	cmb.World = dx::XMMatrixTranspose(m_xWorld);
	m_pMatricesBuffer->CpyData(0, cmb);

	m_pPCML->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	
	///////////////////////
	//			SCENE RTV DRAW
	m_pRTV->StartDraw(m_pPCML);


	m_pPCML->RSSetScissorRects(1, &m_xSceneRect);
	m_pPCML->RSSetViewports(1, &m_xSceneViewPort);

	m_pPCML->ClearDepthStencilView(m_pDSVPack->GetResult()->GetCPUDescriptorHandle(0), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	m_pPCML->ClearRenderTargetView(m_pRTVPack->GetResult()->GetCPUDescriptorHandle(0), COLOR, 0, nullptr);
	m_pPCML->OMSetRenderTargets(1, &FD3DW::keep(m_pRTVPack->GetResult()->GetCPUDescriptorHandle(0)), true, &FD3DW::keep(m_pDSVPack->GetResult()->GetCPUDescriptorHandle(0)));

	m_pPSODefferedFirstPass->Bind(m_pPCML);

	m_pPCML->SetGraphicsRootConstantBufferView(0, m_pMatricesBuffer->GetGPULocation(0));
	m_pPCML->SetGraphicsRootShaderResourceView(3, m_pStructureBufferBones->GetResource()->GetGPUVirtualAddress());

	m_pPCML->IASetVertexBuffers(0, 1, m_pBird->GetVertexBufferView());
	m_pPCML->IASetIndexBuffer(m_pBird->GetIndexBufferView());
	for (size_t ind = 0; ind < m_pBird->GetObjectBuffersCount(); ind++)
	{
		ID3D12DescriptorHeap* heaps[] = { m_vSRVPacks[GetMaterialIndex(m_pBird.get(), ind)]->GetResult()->GetDescriptorPtr(), m_pSamplerPack->GetResult()->GetDescriptorPtr() };
		m_pPCML->SetDescriptorHeaps(_countof(heaps), heaps);
		m_pPCML->SetGraphicsRootDescriptorTable(1, m_vSRVPacks[GetMaterialIndex(m_pBird.get(), ind)]->GetResult()->GetGPUDescriptorHandle(0));
		m_pPCML->SetGraphicsRootDescriptorTable(2, m_pSamplerPack->GetResult()->GetGPUDescriptorHandle(0));

		m_pPCML->DrawIndexedInstanced(GetIndexSize(m_pBird.get(), ind), 1, GetIndexStartPos(m_pBird.get(), ind), GetVertexStartPos(m_pBird.get(), ind), 0);
	}

	m_pRTV->EndDraw(m_pPCML);

	///
	///////////////////////////

	//////////////////////////
	//			MAIN RTV

	BindMainViewPort(m_pPCML);
	BindMainRect(m_pPCML);
	BeginDraw(m_pCommandList->GetPtrCommandList());

	m_pPCML->ClearRenderTargetView(GetCurrBackBufferView(), COLOR, 0, nullptr);
	m_pPCML->OMSetRenderTargets(1, &FD3DW::keep(GetCurrBackBufferView()), true, nullptr);

	m_pPSODefferedSecondPass->Bind(m_pPCML);

	m_pPCML->IASetVertexBuffers(0, 1, m_pScreen->GetVertexBufferView());
	m_pPCML->IASetIndexBuffer(m_pScreen->GetIndexBufferView());

	ID3D12DescriptorHeap* heaps[] = { m_pRTV_SRVPack->GetResult()->GetDescriptorPtr(), m_pSamplerPack->GetResult()->GetDescriptorPtr() };
	m_pPCML->SetDescriptorHeaps(_countof(heaps), heaps);

	m_pPCML->SetGraphicsRootDescriptorTable(0, m_pRTV_SRVPack->GetResult()->GetGPUDescriptorHandle(0));
	m_pPCML->SetGraphicsRootDescriptorTable(1, m_pSamplerPack->GetResult()->GetGPUDescriptorHandle(0));

	m_pPCML->DrawIndexedInstanced(GetIndexSize(m_pScreen.get(), 0), 1, GetIndexStartPos(m_pScreen.get(), 0), GetVertexStartPos(m_pScreen.get(), 0), 0);

	m_pUIComponent->RenderImGui();

	EndDraw(m_pCommandList->GetPtrCommandList());

	ExecuteMainQueue();

}

void MainRenderer::UserClose()
{
	while ( !m_vComponents.empty() ) DestroyComponent( m_vComponents.back().get() );
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
