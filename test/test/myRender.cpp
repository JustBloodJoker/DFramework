#include "myRender.h"

myRender::myRender()
	: DFW(L"MaybeF", 1024, 1024, false)
{

}

myRender::~myRender()
{
	pConstantUploadBuffer.reset();
	pMaterialConstantBuffer.reset();
	samplerPack.reset();
	srvPacks.clear();
	scene.reset();
	screenRes.reset();
	pso.reset();
	rtv.reset();
	dsv.reset();
	dsvPack.reset();
	rtvPack.reset();
	srvPackRTV.reset();
}

void myRender::UserInit()
{
	rtv = std::make_unique<FDW::RenderTarget>(pDevice.Get(), DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RTV_DIMENSION_TEXTURE2D, 1, 1024, 1024, DXGI_SAMPLE_DESC({ 1,0 }));

	srvPackRTV = std::make_unique<FDW::SRVPacker>(cbvsrvuavDescriptorSize, 1u, 0u, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, pDevice.Get());
	srvPackRTV->AddResource(rtv->GetRTVResource(), D3D12_SRV_DIMENSION_TEXTURE2D, 0, pDevice.Get());

	rtvPack = std::make_unique<FDW::RTVPacker>(rtvDescriptorSize, 1u, 0u, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, pDevice.Get());
	rtvPack->AddResource(rtv->GetRTVResource(), rtv->GetRTVDesc(), 0, pDevice.Get());

	dsv = std::make_unique<FDW::DepthStencilView>(pDevice.Get(), DXGI_FORMAT_D24_UNORM_S8_UINT, D3D12_DSV_DIMENSION_TEXTURE2D, 1, 1024, 1024, DXGI_SAMPLE_DESC({ 1,0 }));
	dsvPack = std::make_unique<FDW::DSVPacker>(dsvDescriptorSize, 1u, 0u, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, pDevice.Get());
	dsvPack->AddResource(dsv->GetDSVResource(), dsv->GetDSVDesc(), 0, pDevice.Get());
	dsv->DepthWrite(pCommandList.Get());

	screenRes = std::make_unique<FDW::Rectangle>(pDevice.Get(), pCommandList.Get(), true);

	scene = std::make_unique<FDW::Scene>("scene.gltf", pDevice.Get(), pCommandList.Get(), true);
	
	pConstantUploadBuffer = std::make_unique<FDW::UploadBuffer<FDW::MatricesConstantBufferStructureFrameWork>>(pDevice.Get(), 1, true);
	pMaterialConstantBuffer = std::make_unique<FDW::UploadBuffer<FDW::MaterialFrameWork>>(pDevice.Get(), scene->GetMaterialSize(), true);
	

	for (size_t ind = 0; ind < scene->GetMaterialSize(); ind++)
	{
		srvPacks.push_back(std::make_unique<FDW::SRVPacker>(cbvsrvuavDescriptorSize, 1u, 0u, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, pDevice.Get()));
		if (scene->GetMaterialMananger()->GetMaterial(ind)->IsHaveTexture(FDW::BASE))
		{
			(*srvPacks.rbegin())->AddResource(scene->GetMaterialMananger()->GetMaterial(ind)->GetResourceTexture(FDW::TEXTURE_TYPE::BASE), D3D12_SRV_DIMENSION_TEXTURE2D, 0, pDevice.Get());
		}
	}

	samplerPack = std::make_unique<FDW::SamplerPacker>(cbvsrvuavDescriptorSize, 1u, 0u, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, pDevice.Get());	
	samplerPack->AddDefaultSampler(0, pDevice.Get());

	for (size_t ind = 0; ind < scene->GetMaterialSize(); ind++)
	{
		pMaterialConstantBuffer->CpyData(ind, scene->GetMaterialMananger()->GetMaterialDesc(ind));
	}
	

	std::vector<D3D12_INPUT_ELEMENT_DESC> layoutDesc =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 44, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};





	CD3DX12_ROOT_PARAMETER slotRootParameter[4];
	slotRootParameter[0].InitAsConstantBufferView(0);
	slotRootParameter[1].InitAsConstantBufferView(1);
	
	CD3DX12_DESCRIPTOR_RANGE srvTable;
	srvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0);
	slotRootParameter[2].InitAsDescriptorTable(1, &srvTable, D3D12_SHADER_VISIBILITY_PIXEL);
	
	CD3DX12_DESCRIPTOR_RANGE samplerTable;
	samplerTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);
	slotRootParameter[3].InitAsDescriptorTable(1, &samplerTable, D3D12_SHADER_VISIBILITY_PIXEL);


	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4,
		slotRootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	wrl::ComPtr<ID3DBlob> serializedRootSig = nullptr;
	wrl::ComPtr<ID3DBlob> errorBlob = nullptr;

	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	HRESULT_ASSERT(pDevice->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(pRootSignature.GetAddressOf())), "Root signature create error");


	wrl::ComPtr<ID3DBlob> pVSByteCode;
	wrl::ComPtr<ID3DBlob> pPSByteCode;
	FDW::Shader::GenerateBytecode(L"shaders/simpleShader.hlsl",nullptr, "VS", "vs_5_1", pVSByteCode);
	FDW::Shader::GenerateBytecode(L"shaders/simpleShader.hlsl",nullptr, "PS", "ps_5_1", pPSByteCode);

	CD3DX12_RASTERIZER_DESC rasterizerDesc(D3D12_DEFAULT);
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
	rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
	
	DXGI_FORMAT rtvFormats[]{ DXGI_FORMAT_R8G8B8A8_UNORM };
	pso = std::make_unique<FDW::PipelineStateObject>(pRootSignature.Get(), layoutDesc.data(), layoutDesc.size(), 1, rtvFormats , DXGI_FORMAT_D24_UNORM_S8_UINT);
	pso->SetVS(pVSByteCode.Get());
	pso->SetPS(pPSByteCode.Get());
	pso->SetRasterizerState(rasterizerDesc);
	pso->CreatePSO(pDevice.Get());


	
	eye = dx::XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f);
	at = dx::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	up = dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	pCommandList->Close();
	ImmediateExecuteQueue(pCommandList.Get());

	FDW::Texture::ReleaseUploadBuffers();
}

void myRender::UserLoop()
{
	FLOAT COLOR[4] = { 0.0F, 1.0F, 0.0F, 1.0F };
	
	at = dx::XMVector3TransformCoord(dx::XMVectorSet(0.0f, 0.0f, 0.01f, 0.0f),
		dx::XMMatrixRotationRollPitchYaw(camPitch, camYaw, 0));
	at = dx::XMVector3Normalize(at);
	up = dx::XMVector3TransformCoord(up, dx::XMMatrixRotationY(camYaw));
	at = dx::XMVectorAdd(at, eye);
	
	view = dx::XMMatrixLookAtLH(eye, at, up);

	world = dx::XMMatrixIdentity();

	FDW::MatricesConstantBufferStructureFrameWork cmb;
	cmb.projection = dx::XMMatrixTranspose(mainProjectionMatrix);
	cmb.view = dx::XMMatrixTranspose(view);
	cmb.world = dx::XMMatrixTranspose(world);

	pConstantUploadBuffer->CpyData(0, cmb);

	pCommandList->SetPipelineState(pso->GetPSO());
	pCommandList->SetGraphicsRootSignature(pRootSignature.Get());
	
	pCommandList->SetGraphicsRootConstantBufferView(0, pConstantUploadBuffer->GetResource()->GetGPUVirtualAddress());

	pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	///////////////////////
	// MAIN RTV DRAW

	ID3D12DescriptorHeap* dcHeaps[] =
	{
		srvPackRTV->GetResult()->GetDescriptorPtr(),
		samplerPack->GetResult()->GetDescriptorPtr()
	};
	pCommandList->SetDescriptorHeaps(_countof(dcHeaps), dcHeaps);
	pCommandList->SetGraphicsRootDescriptorTable(2, srvPackRTV->GetResult()->GetGPUDescriptorHandle(0));
	pCommandList->SetGraphicsRootDescriptorTable(3, samplerPack->GetResult()->GetGPUDescriptorHandle(0));
	pCommandList->IASetVertexBuffers(0, 1, screenRes->GetVertexBufferView());
	pCommandList->IASetIndexBuffer(screenRes->GetIndexBufferView());
	pCommandList->DrawIndexedInstanced(std::get<2>(screenRes->GetObjectParameters(0)), 1, std::get<3>(screenRes->GetObjectParameters(0)), std::get<1>(screenRes->GetObjectParameters(0)), 0);

	///////////////////////
	// USER RTV DRAW
	rtv->StartDraw(pCommandList.Get());
	
	pCommandList->ClearDepthStencilView(dsvPack->GetResult()->GetCPUDescriptorHandle(0), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	pCommandList->ClearRenderTargetView(rtvPack->GetResult()->GetCPUDescriptorHandle(0), COLOR, 0, nullptr);
	pCommandList->OMSetRenderTargets(1, &FDW::keep(rtvPack->GetResult()->GetCPUDescriptorHandle(0)), true, &FDW::keep(dsvPack->GetResult()->GetCPUDescriptorHandle(0)));

	pCommandList->IASetVertexBuffers(0, 1, scene->GetVertexBufferView());
	pCommandList->IASetIndexBuffer(scene->GetIndexBufferView());
	for (size_t ind = 0; ind < scene->GetObjectBuffersCount(); ind++)
	{
		ID3D12DescriptorHeap* dcHeaps[] =
		{
			srvPacks[std::get<4>(scene->GetObjectParameters(ind))]->GetResult()->GetDescriptorPtr(),
			samplerPack->GetResult()->GetDescriptorPtr()
		};
		pCommandList->SetDescriptorHeaps(_countof(dcHeaps), dcHeaps);
		pCommandList->SetGraphicsRootDescriptorTable(2, srvPacks[std::get<4>(scene->GetObjectParameters(ind))]->GetResult()->GetGPUDescriptorHandle(0));
		pCommandList->SetGraphicsRootDescriptorTable(3, samplerPack->GetResult()->GetGPUDescriptorHandle(0));

		pCommandList->SetGraphicsRootConstantBufferView(1, pMaterialConstantBuffer->GetResource()->GetGPUVirtualAddress() + std::get<4>(scene->GetObjectParameters(ind)) * pMaterialConstantBuffer->GetDataSize());
		
		UINT indexSize = std::get<2>(scene->GetObjectParameters(ind));
		UINT indexstart = std::get<3>(scene->GetObjectParameters(ind));
		UINT vertexStart = std::get<1>(scene->GetObjectParameters(ind));

		pCommandList->DrawIndexedInstanced(indexSize, 1, indexstart, vertexStart, 0);
	}
	
	rtv->EndDraw(pCommandList.Get());
}

void myRender::UserClose()
{
}

void myRender::UserMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;
	SetCapture(hwnd);
}

void myRender::UserMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void myRender::UserMouseMoved(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0 && ((x != mLastMousePos.x) || (y != mLastMousePos.y)))
	{
		camYaw += (mLastMousePos.x - x) * 0.002;
		camPitch += (mLastMousePos.y - y) * 0.002;
	}
	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void myRender::UserKeyPressed(WPARAM wParam)
{
	dx::XMVECTOR cameraDirection = dx::XMVector3Normalize(dx::XMVectorSubtract(at, eye));
	dx::XMVECTOR rightDirection = dx::XMVector3Normalize(dx::XMVector3Cross(dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), cameraDirection));
	
	if (wParam == 'W')
	{
		eye = dx::XMVectorAdd(eye, dx::XMVectorScale(cameraDirection, 10.0f * 0.016f));
	}
	if (wParam == 'S')
	{
		eye = dx::XMVectorSubtract(eye, dx::XMVectorScale(cameraDirection, 10.0f * 0.016f));
	}
	if (wParam == 'D')
	{
		eye = dx::XMVectorAdd(eye, dx::XMVectorScale(rightDirection, 10.0f * 0.016f));
	}
	if (wParam == 'A')
	{
		eye = dx::XMVectorSubtract(eye, dx::XMVectorScale(rightDirection, 10.0f * 0.016f));
	}
}

