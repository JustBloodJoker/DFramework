#include "AnimationTestRender.h"

static FLOAT COLOR[4] = { 0.2f,0.2f,0.2f,1.0f };

AnimationTestRender::AnimationTestRender()
	: WinWindow(L"MaybeF", 1024, 1024, false)
{

}

AnimationTestRender::~AnimationTestRender()
{

}

void AnimationTestRender::UserInit()
{
	//SetVSync(true);

	m_pCommandList = CreateList(D3D12_COMMAND_LIST_TYPE_DIRECT);
	m_pPCML = m_pCommandList->GetPtrCommandList();

	m_pTimer = GetTimer();
	auto device = GetDevice();

	BindListToMainQueue(m_pCommandList.get());

	m_pBird = CreateScene("D3DFrameworkTest/sampleModels/bird/scene.gltf", true, m_pPCML);

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

	m_pStructureBufferBones = CreateSimpleStructuredBuffer(m_pBird->GetBonesCount() * sizeof(dx::XMMATRIX));

	m_xEye = dx::XMVectorSet(0.0f, 0.0f, -1.0f, 1.0f);
	m_xAt = dx::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	m_xStartUp = dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	m_pMatricesBuffer = CreateConstantBuffer<FD3DW::MatricesConstantBufferStructureFrameWork>(1);

	CD3DX12_ROOT_PARAMETER slotRootParameter[4];
	slotRootParameter[0].InitAsConstantBufferView(0);
	slotRootParameter[1].InitAsDescriptorTable(1, &FD3DW::keep(CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0)));
	slotRootParameter[2].InitAsDescriptorTable(1, &FD3DW::keep(CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0)));
	slotRootParameter[3].InitAsShaderResourceView(1);

	m_pRootSignnatureRender = CreateRootSignature(slotRootParameter, ARRAYSIZE(slotRootParameter));

	CD3DX12_RASTERIZER_DESC rasterizerDesc(D3D12_DEFAULT);
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
	rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;

	wrl::ComPtr<ID3DBlob> pVSByteCode;
	wrl::ComPtr<ID3DBlob> pPSByteCode;

	FD3DW::Shader::LoadBytecode(L"D3DFrameworkTest/shaderBytecode/simpleShaderPS.shader", pPSByteCode);
	FD3DW::Shader::LoadBytecode(L"D3DFrameworkTest/shaderBytecode/simpleShaderVS.shader", pVSByteCode);


	auto wndSettins = GetMainWNDSettings();
	m_pRTV = CreateRenderTarget(DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RTV_DIMENSION_TEXTURE2D, 1, wndSettins.Width, wndSettins.Height);
	m_pRTVPack = CreateRTVPack(1u);
	m_pRTVPack->PushResource(m_pRTV->GetRTVResource(), m_pRTV->GetRTVDesc(), device);
	m_pRTV_SRVPack = CreateSRVPack(1u);
	m_pRTV_SRVPack->PushResource(m_pRTV->GetRTVResource(), D3D12_SRV_DIMENSION_TEXTURE2D, device);

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

	DXGI_FORMAT rtvFormats[]{ m_pRTV->GetRTVDesc().Format };
	m_pPSO = CreatePSO(m_pRootSignnatureRender->GetRootSignature(), scenelayoutDesc.data(), (UINT)scenelayoutDesc.size(),
		_countof(rtvFormats), rtvFormats, DXGI_FORMAT_D24_UNORM_S8_UINT, UINT_MAX, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
		pVSByteCode.Get(), pPSByteCode.Get(), nullptr, nullptr, nullptr, rasterizerDesc);


	m_pScreen = CreateRectangle(true, m_pPCML);

	wrl::ComPtr<ID3DBlob> pVSByteCode1;
	wrl::ComPtr<ID3DBlob> pPSByteCode1;

	FD3DW::Shader::LoadBytecode(L"D3DFrameworkTest/shaderBytecode/ScreenPostProcessPS.shader", pPSByteCode1);
	FD3DW::Shader::LoadBytecode(L"D3DFrameworkTest/shaderBytecode/ScreenPostProcessVS.shader", pVSByteCode1);


	CD3DX12_ROOT_PARAMETER slotRootParameter1[2];
	slotRootParameter1[0].InitAsDescriptorTable(1, &FD3DW::keep(CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0)));
	slotRootParameter1[1].InitAsDescriptorTable(1, &FD3DW::keep(CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0)));
	m_pRootScreen = CreateRootSignature(slotRootParameter1, _countof(slotRootParameter1));

	std::vector<D3D12_INPUT_ELEMENT_DESC> ScreenlayoutDesc =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	DXGI_FORMAT rtvFormats1[]{ GetMainRTVFormat() };
	m_pPSOScreen = CreatePSO(m_pRootScreen->GetRootSignature(), ScreenlayoutDesc.data(), (UINT)ScreenlayoutDesc.size(),
		_countof(rtvFormats1), rtvFormats1, DXGI_FORMAT_D24_UNORM_S8_UINT, UINT_MAX, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
		pVSByteCode1.Get(), pPSByteCode1.Get(), nullptr, nullptr, nullptr, rasterizerDesc);



	ExecuteMainQueue();
	FD3DW::FResource::ReleaseUploadBuffers();
}

void AnimationTestRender::UserLoop()
{
	m_pCommandList->ResetList();

	m_xAt = dx::XMVectorAdd(dx::XMVector3Normalize(dx::XMVector3TransformCoord(dx::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f),
		dx::XMMatrixRotationRollPitchYaw(m_fCamPitch, m_fCamYaw, 0))), m_xEye);

	m_xUp = dx::XMVector3Normalize(dx::XMVector3TransformCoord(m_xStartUp, dx::XMMatrixRotationRollPitchYaw(0, 0, m_fCamRoll)));
	m_xView = dx::XMMatrixLookAtLH(m_xEye, m_xAt, m_xUp);
	m_xWorld = dx::XMMatrixIdentity();

	auto resultVector = m_pBird->PlayAnimation(m_pTimer->GetTime(), "Take 001");
	m_pStructureBufferBones->UploadData(GetDevice(), m_pPCML, resultVector.data());

	FD3DW::MatricesConstantBufferStructureFrameWork cmb;
	cmb.Projection = dx::XMMatrixTranspose(GetMainProjectionMatrix());
	cmb.View = dx::XMMatrixTranspose(m_xView);
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

	m_pPCML->SetPipelineState(m_pPSO->GetPSO());
	m_pPCML->SetGraphicsRootSignature(m_pRootSignnatureRender->GetRootSignature());

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

	m_pPCML->ClearRenderTargetView(GetCurrBackBufferView(), COLOR, 0, nullptr);
	m_pPCML->OMSetRenderTargets(1, &FD3DW::keep(GetCurrBackBufferView()), true, nullptr);

	m_pPCML->SetPipelineState(m_pPSOScreen->GetPSO());
	m_pPCML->SetGraphicsRootSignature(m_pRootScreen->GetRootSignature());

	m_pPCML->IASetVertexBuffers(0, 1, m_pScreen->GetVertexBufferView());
	m_pPCML->IASetIndexBuffer(m_pScreen->GetIndexBufferView());

	ID3D12DescriptorHeap* heaps[] = { m_pRTV_SRVPack->GetResult()->GetDescriptorPtr(), m_pSamplerPack->GetResult()->GetDescriptorPtr() };
	m_pPCML->SetDescriptorHeaps(_countof(heaps), heaps);

	m_pPCML->SetGraphicsRootDescriptorTable(0, m_pRTV_SRVPack->GetResult()->GetGPUDescriptorHandle(0));
	m_pPCML->SetGraphicsRootDescriptorTable(1, m_pSamplerPack->GetResult()->GetGPUDescriptorHandle(0));

	m_pPCML->DrawIndexedInstanced(GetIndexSize(m_pScreen.get(), 0), 1, GetIndexStartPos(m_pScreen.get(), 0), GetVertexStartPos(m_pScreen.get(), 0), 0);
}

void AnimationTestRender::UserClose()
{
}

void AnimationTestRender::UserMouseDown(WPARAM btnState, int x, int y)
{
	m_xLastMousePos.x = x;
	m_xLastMousePos.y = y;
}

void AnimationTestRender::UserMouseUp(WPARAM btnState, int x, int y)
{
}

void AnimationTestRender::UserMouseMoved(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0 && ((x != m_xLastMousePos.x) || (y != m_xLastMousePos.y)))
	{
		m_fCamYaw += (m_xLastMousePos.x - x) * 0.002f;
		m_fCamPitch += (m_xLastMousePos.y - y) * 0.002f;
	}
	if ((btnState & MK_RBUTTON) != 0 && ((x != m_xLastMousePos.x) || (y != m_xLastMousePos.y)))
	{
		m_fCamRoll += (m_xLastMousePos.x - x) * 0.002f;
	}
	m_xLastMousePos.x = x;
	m_xLastMousePos.y = y;
}

void AnimationTestRender::UserKeyPressed(WPARAM wParam)
{
	dx::XMVECTOR cameraDirection = dx::XMVector3Normalize(dx::XMVectorSubtract(m_xAt, m_xEye));
	dx::XMVECTOR rightDirection = dx::XMVector3Normalize(dx::XMVector3Cross(m_xUp, cameraDirection));

	if (wParam == 'W')
	{
		m_xEye = dx::XMVectorAdd(m_xEye, dx::XMVectorScale(cameraDirection, 1000.0f * 0.016f));
	}
	if (wParam == 'S')
	{
		m_xEye = dx::XMVectorSubtract(m_xEye, dx::XMVectorScale(cameraDirection, 1000.0f * 0.016f));
	}
	if (wParam == 'D')
	{
		m_xEye = dx::XMVectorAdd(m_xEye, dx::XMVectorScale(rightDirection, 1000.0f * 0.016f));
	}
	if (wParam == 'A')
	{
		m_xEye = dx::XMVectorSubtract(m_xEye, dx::XMVectorScale(rightDirection, 1000.0f * 0.016f));
	}

	if (wParam == 'R')
	{
		m_fCamRoll = 0.0f;
	}
}

void AnimationTestRender::UserResizeUpdate()
{
	const auto& WndSet = WNDSettings();
	m_xProjectionMatrix = dx::XMMatrixPerspectiveFovLH(M_PI_2_F, (float)WndSet.Width / WndSet.Height, 1.0f, 10000.0f);
}

void AnimationTestRender::UserEndResizeUpdate()
{
}
