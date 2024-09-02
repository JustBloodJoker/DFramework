#include "myRender.h"

static FLOAT COLOR[4] = { 0.2f,0.2f,0.2f,1.0f };

myRender::myRender()
	: DFW(L"MaybeF", 1024, 1024, false)
{

}

myRender::~myRender()
{

}

void myRender::UserInit()
{
	SetVSync(true);

	pCommandList = CreateList(D3D12_COMMAND_LIST_TYPE_DIRECT);
	pcml = pCommandList->GetPtrCommandList();


//	if (InitNGX(pCommandList.get()))
//	{
//		CONSOLE_MESSAGE("NGX inited");
//	}


	timer = GetTimer();
	auto device = GetDevice();
	
	music = CreateAudio(L"322.wav");
	music->SetVolume(0.1f);

	
	BindListToMainQueue(pCommandList.get());

	bird = CreateScene("sampleModels/bird/scene.gltf", true, pcml);
	
	for (size_t ind = 0; ind < bird->GetMaterialSize(); ind++)
	{
		srvPacks.push_back(CreateSRVPack(1u));
		if (bird->GetMaterialMananger()->GetMaterial(ind)->IsHaveTexture(TextureType::BASE))
		{
			(*srvPacks.rbegin())->PushResource(bird->GetMaterialMananger()->GetMaterial(ind)->GetResourceTexture(TextureType::BASE), D3D12_SRV_DIMENSION_TEXTURE2D, device);
		}
	}
	samplerPack = CreateSamplerPack(1u);
	samplerPack->PushDefaultSampler(device);

	dsv = CreateDepthStencilView(DXGI_FORMAT_D24_UNORM_S8_UINT, D3D12_DSV_DIMENSION_TEXTURE2D, 1, 1024, 1024);
	dsvPack = CreateDSVPack(1u);
	dsvPack->PushResource(dsv->GetDSVResource(), dsv->GetDSVDesc(), device);

	std::vector<D3D12_INPUT_ELEMENT_DESC> scenelayoutDesc =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	for (int i = 0; i < NUM_BONES_PER_VEREX; i++)
	{
		scenelayoutDesc.emplace_back(D3D12_INPUT_ELEMENT_DESC({ "IDS_BONES", (UINT)i, DXGI_FORMAT_R32_UINT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }));
	}
	for (int i = 0; i < NUM_BONES_PER_VEREX; i++)
	{
		scenelayoutDesc.emplace_back(D3D12_INPUT_ELEMENT_DESC({ "WEIGHT_BONES", (UINT)i, DXGI_FORMAT_R32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }));
	}

	structureBufferBones = CreateSimpleStructuredBuffer(bird->GetBonesCount() * sizeof(dx::XMMATRIX));

	eye = dx::XMVectorSet(0.0f, 0.0f, -1.0f, 1.0f);
	at = dx::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	startUp = dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	pMatricesBuffer = CreateConstantBuffer<FDW::MatricesConstantBufferStructureFrameWork>(1);

	CD3DX12_ROOT_PARAMETER slotRootParameter[4];
	slotRootParameter[0].InitAsConstantBufferView(0);
	slotRootParameter[1].InitAsDescriptorTable(1, &FDW::keep(CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0)));
	slotRootParameter[2].InitAsDescriptorTable(1, &FDW::keep(CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0)));
	slotRootParameter[3].InitAsShaderResourceView(1);

	pRootSignnatureRender = CreateRootSignature(slotRootParameter, ARRAYSIZE(slotRootParameter));

	CD3DX12_RASTERIZER_DESC rasterizerDesc(D3D12_DEFAULT);
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
	rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;

	wrl::ComPtr<ID3DBlob> pVSByteCode;
	wrl::ComPtr<ID3DBlob> pPSByteCode;
	
	//FDW::Shader::GenerateBytecode(L"shaders/simpleShader.hlsl", nullptr, "VS", "vs_5_1", pVSByteCode);
	//FDW::Shader::GenerateBytecode(L"shaders/simpleShader.hlsl", nullptr, "PS", "ps_5_1", pPSByteCode);
 	//FDW::Shader::SaveBytecode(L"shaderBytecode/simpleShaderPS.shader", pPSByteCode);
	//FDW::Shader::SaveBytecode(L"shaderBytecode/simpleShaderVS.shader", pVSByteCode);

	FDW::Shader::LoadBytecode(L"shaderBytecode/simpleShaderPS.shader", pPSByteCode);
	FDW::Shader::LoadBytecode(L"shaderBytecode/simpleShaderVS.shader", pVSByteCode);


	auto wndSettins = GetMainWNDSettings();
	rtv = CreateRenderTarget(DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RTV_DIMENSION_TEXTURE2D, 1, wndSettins.width, wndSettins.height); //wndSettins.dlssWidth, wndSettins.dlssHeight)
	rtvPack = CreateRTVPack(1u);
	rtvPack->PushResource(rtv->GetRTVResource(), rtv->GetRTVDesc(), device);
	rtvSrvPack = CreateSRVPack(1u);
	rtvSrvPack->PushResource(rtv->GetRTVResource(), D3D12_SRV_DIMENSION_TEXTURE2D, device);

	sceneViewPort.MaxDepth = 1.0f;
	sceneViewPort.MinDepth = 0.0f;
	sceneViewPort.Height = wndSettins.height; //wndSettins.dlssHeight
	sceneViewPort.Width = wndSettins.width;//wndSettins.dlssWidth;
	sceneViewPort.TopLeftX = 0;
	sceneViewPort.TopLeftY = 0;

	sceneRect.left = 0;
	sceneRect.right = wndSettins.width; //wndSettins.dlssWidth
	sceneRect.top = 0;
	sceneRect.bottom = wndSettins.height; //wndSettins.dlssHeight;

	DXGI_FORMAT rtvFormats[]{ rtv->GetRTVDesc().Format };
	pso = CreatePSO(pRootSignnatureRender->GetRootSignature(), scenelayoutDesc.data(), (UINT)scenelayoutDesc.size(),
		_countof(rtvFormats), rtvFormats, DXGI_FORMAT_D24_UNORM_S8_UINT, UINT_MAX, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
		pVSByteCode.Get(), pPSByteCode.Get(), nullptr, nullptr, nullptr, rasterizerDesc);


	screen = CreateRectangle(true, pcml);

	wrl::ComPtr<ID3DBlob> pVSByteCode1;
	wrl::ComPtr<ID3DBlob> pPSByteCode1;

//	FDW::Shader::GenerateBytecode(L"shaders/ScreenPostProcess.hlsl", nullptr, "VS", "vs_5_1", pVSByteCode1);
//	FDW::Shader::GenerateBytecode(L"shaders/ScreenPostProcess.hlsl", nullptr, "PS", "ps_5_1", pPSByteCode1);
//	FDW::Shader::SaveBytecode(L"shaderBytecode/ScreenPostProcessPS.shader", pPSByteCode1);
//	FDW::Shader::SaveBytecode(L"shaderBytecode/ScreenPostProcessVS.shader", pVSByteCode1);

	FDW::Shader::LoadBytecode(L"shaderBytecode/ScreenPostProcessPS.shader", pPSByteCode1);
	FDW::Shader::LoadBytecode(L"shaderBytecode/ScreenPostProcessVS.shader", pVSByteCode1);


	CD3DX12_ROOT_PARAMETER slotRootParameter1[2];
	slotRootParameter1[0].InitAsDescriptorTable(1, &FDW::keep(CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0)));
	slotRootParameter1[1].InitAsDescriptorTable(1, &FDW::keep(CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0)));
	rootScreen = CreateRootSignature(slotRootParameter1, _countof(slotRootParameter1));

	std::vector<D3D12_INPUT_ELEMENT_DESC> ScreenlayoutDesc =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	DXGI_FORMAT rtvFormats1[]{ GetMainRTVFormat() };
	psoScreen = CreatePSO(rootScreen->GetRootSignature(), ScreenlayoutDesc.data(), (UINT)ScreenlayoutDesc.size(),
		_countof(rtvFormats1), rtvFormats1, DXGI_FORMAT_D24_UNORM_S8_UINT, UINT_MAX, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
		pVSByteCode1.Get(), pPSByteCode1.Get(), nullptr, nullptr, nullptr, rasterizerDesc);



	ExecuteMainQueue();
	FDW::Texture::ReleaseUploadBuffers();

	pp = std::make_unique<FDW::PostProcessing>( rtv->GetRTVResource() );
}

void myRender::UserLoop()
{
	music->Play();

	pCommandList->ResetList();
	


	at = dx::XMVectorAdd(dx::XMVector3Normalize(dx::XMVector3TransformCoord(dx::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f),
		dx::XMMatrixRotationRollPitchYaw(camPitch, camYaw, 0))), eye);

	up = dx::XMVector3Normalize(dx::XMVector3TransformCoord(startUp, dx::XMMatrixRotationRollPitchYaw(0, 0, camRoll)));
	view = dx::XMMatrixLookAtLH(eye, at, up);
	world = dx::XMMatrixIdentity();

	auto resultVector = bird->PlayAnimation(timer->GetTime(), "Take 001");
	structureBufferBones->UploadData(GetDevice(), pcml, resultVector.data());

	FDW::MatricesConstantBufferStructureFrameWork cmb;
	cmb.projection = dx::XMMatrixTranspose(GetMainProjectionMatrix());
	cmb.view = dx::XMMatrixTranspose(view);
	cmb.world = dx::XMMatrixTranspose(world);
	pMatricesBuffer->CpyData(0, cmb);

	pcml->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	
	///////////////////////
	//			SCENE RTV DRAW
	rtv->StartDraw(pcml);


	pcml->RSSetScissorRects(1, &sceneRect);
	pcml->RSSetViewports(1, &sceneViewPort);

	pcml->ClearDepthStencilView(dsvPack->GetResult()->GetCPUDescriptorHandle(0), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	pcml->ClearRenderTargetView(rtvPack->GetResult()->GetCPUDescriptorHandle(0), COLOR, 0, nullptr);
	pcml->OMSetRenderTargets(1, &FDW::keep(rtvPack->GetResult()->GetCPUDescriptorHandle(0)), true, &FDW::keep(dsvPack->GetResult()->GetCPUDescriptorHandle(0)));

	pcml->SetPipelineState(pso->GetPSO());
	pcml->SetGraphicsRootSignature(pRootSignnatureRender->GetRootSignature());

	pcml->SetGraphicsRootConstantBufferView(0, pMatricesBuffer->GetGPULocation(0));
	pcml->SetGraphicsRootShaderResourceView(3, structureBufferBones->GetResource()->GetGPUVirtualAddress());

	pcml->IASetVertexBuffers(0, 1, bird->GetVertexBufferView());
	pcml->IASetIndexBuffer(bird->GetIndexBufferView());
	for (size_t ind = 0; ind < bird->GetObjectBuffersCount(); ind++)
	{
		ID3D12DescriptorHeap* heaps[] = { srvPacks[GetMaterialIndex(bird.get(), ind)]->GetResult()->GetDescriptorPtr(), samplerPack->GetResult()->GetDescriptorPtr() };
		pcml->SetDescriptorHeaps(_countof(heaps), heaps);
		pcml->SetGraphicsRootDescriptorTable(1, srvPacks[GetMaterialIndex(bird.get(), ind)]->GetResult()->GetGPUDescriptorHandle(0));
		pcml->SetGraphicsRootDescriptorTable(2, samplerPack->GetResult()->GetGPUDescriptorHandle(0));
		
		pcml->DrawIndexedInstanced(GetIndexSize(bird.get(), ind), 1, GetIndexStartPos(bird.get(), ind), GetVertexStartPos(bird.get(), ind), 0);
	}

	rtv->EndDraw(pcml);

	ExecuteMainQueue();
	pCommandList->ResetList();

	pp->InverseEffect(GetDevice());
	///
	///////////////////////////
	
	//////////////////////////
	//			MAIN RTV

	BeginDraw(pcml);
	BindMainViewPort(pcml);
	BindMainRect(pcml);

	pcml->ClearRenderTargetView(GetCurrBackBufferView(), COLOR, 0, nullptr);
	pcml->OMSetRenderTargets(1, &FDW::keep(GetCurrBackBufferView()), true, nullptr);

	pcml->SetPipelineState(psoScreen->GetPSO());
	pcml->SetGraphicsRootSignature(rootScreen->GetRootSignature());

	pcml->IASetVertexBuffers(0, 1, screen->GetVertexBufferView());
	pcml->IASetIndexBuffer(screen->GetIndexBufferView());
	
	ID3D12DescriptorHeap* heaps[] = { rtvSrvPack->GetResult()->GetDescriptorPtr(), samplerPack->GetResult()->GetDescriptorPtr() };
	pcml->SetDescriptorHeaps(_countof(heaps), heaps);

	pcml->SetGraphicsRootDescriptorTable(0, rtvSrvPack->GetResult()->GetGPUDescriptorHandle(0));
	pcml->SetGraphicsRootDescriptorTable(1, samplerPack->GetResult()->GetGPUDescriptorHandle(0));

	pcml->DrawIndexedInstanced(GetIndexSize(screen.get(), 0), 1, GetIndexStartPos(screen.get(), 0), GetVertexStartPos(screen.get(), 0), 0);

	EndDraw(pcml);
}

void myRender::UserClose()
{
}

void myRender::UserMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;
	SetCapture(GetMainHWND());
}

void myRender::UserMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void myRender::UserMouseMoved(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0 && ((x != mLastMousePos.x) || (y != mLastMousePos.y)))
	{
		camYaw += (mLastMousePos.x - x) * 0.002f;
		camPitch += (mLastMousePos.y - y) * 0.002f;
	}
	if ((btnState & MK_RBUTTON) != 0 && ((x != mLastMousePos.x) || (y != mLastMousePos.y)))
	{
		camRoll += (mLastMousePos.x - x) * 0.002f;
	}
	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void myRender::UserKeyPressed(WPARAM wParam)
{
	dx::XMVECTOR cameraDirection = dx::XMVector3Normalize(dx::XMVectorSubtract(at, eye));
	dx::XMVECTOR rightDirection = dx::XMVector3Normalize(dx::XMVector3Cross(up, cameraDirection));
	
	if (wParam == 'W')
	{
		eye = dx::XMVectorAdd(eye, dx::XMVectorScale(cameraDirection, 1000.0f * 0.016f));
	}
	if (wParam == 'S')
	{
		eye = dx::XMVectorSubtract(eye, dx::XMVectorScale(cameraDirection, 1000.0f * 0.016f));
	}
	if (wParam == 'D')
	{
		eye = dx::XMVectorAdd(eye, dx::XMVectorScale(rightDirection, 1000.0f * 0.016f));
	}
	if (wParam == 'A')
	{
		eye = dx::XMVectorSubtract(eye, dx::XMVectorScale(rightDirection, 1000.0f * 0.016f));
	}

	if (wParam == 'R')
	{
		camRoll = 0.0f;
	}
}

void myRender::UserResizeUpdate()
{
	//OnResizeDLSS(pCommandList.get());
}
 