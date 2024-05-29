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
	rtvPos.reset();
	rtvBase.reset();
	rtvNormals.reset();
	dsv.reset();
	dsvPack.reset();
	rtvPack.reset();
	srvPackRTV.reset();
	pRootSignFirstPass.reset();
	pRootSignSecondPass.reset();
}

void myRender::UserInit()
{
	rtvPos = CreateRenderTarget(DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RTV_DIMENSION_TEXTURE2D, 1, 1024, 1024);
	rtvBase = CreateRenderTarget(DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RTV_DIMENSION_TEXTURE2D, 1, 1024, 1024);
	rtvNormals = CreateRenderTarget(DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RTV_DIMENSION_TEXTURE2D, 1, 1024, 1024);
	
	srvPackRTV = CreateSRVPack(3u);
	srvPackRTV->PushResource(rtvPos->GetRTVResource(), D3D12_SRV_DIMENSION_TEXTURE2D, pDevice.Get());
	srvPackRTV->PushResource(rtvBase->GetRTVResource(), D3D12_SRV_DIMENSION_TEXTURE2D, pDevice.Get());
	srvPackRTV->PushResource(rtvNormals->GetRTVResource(), D3D12_SRV_DIMENSION_TEXTURE2D, pDevice.Get());

	rtvPack = CreateRTVPack(3u);
	rtvPack->PushResource(rtvPos->GetRTVResource(), rtvPos->GetRTVDesc(), pDevice.Get());
	rtvPack->PushResource(rtvBase->GetRTVResource(), rtvBase->GetRTVDesc(), pDevice.Get());
	rtvPack->PushResource(rtvNormals->GetRTVResource(), rtvNormals->GetRTVDesc(), pDevice.Get());
	
	dsv = CreateDepthStencilView(DXGI_FORMAT_D24_UNORM_S8_UINT, D3D12_DSV_DIMENSION_TEXTURE2D, 1, 1024, 1024);
	
	dsvPack = CreateDSVPack(1u);
	dsvPack->PushResource(dsv->GetDSVResource(), dsv->GetDSVDesc(), pDevice.Get());
	dsv->DepthWrite(pCommandList.Get());

	screenRes = CreateRectangle(true);
	scene = CreateScene("sponza/sponza.gltf", true);
	
	pConstantUploadBuffer = CreateConstantBuffer<FDW::MatricesConstantBufferStructureFrameWork>(1);
	pMaterialConstantBuffer = CreateConstantBuffer<FDW::MaterialFrameWork>(scene->GetMaterialSize());

	for (size_t ind = 0; ind < scene->GetMaterialSize(); ind++)
	{
		srvPacks.push_back(CreateSRVPack(1u));
		if (scene->GetMaterialMananger()->GetMaterial(ind)->IsHaveTexture(FDW::BASE))
		{
			(*srvPacks.rbegin())->PushResource(scene->GetMaterialMananger()->GetMaterial(ind)->GetResourceTexture(FDW::TEXTURE_TYPE::BASE), D3D12_SRV_DIMENSION_TEXTURE2D, pDevice.Get());
		}
		pMaterialConstantBuffer->CpyData(ind, scene->GetMaterialMananger()->GetMaterialDesc(ind));
	}

	samplerPack = CreateSamplerPack(1u);
	samplerPack->AddDefaultSampler(0, pDevice.Get());
	
	
	std::vector<D3D12_INPUT_ELEMENT_DESC> layoutDesc =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 44, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	CD3DX12_ROOT_PARAMETER slotMainRootParameter[2];

	CD3DX12_DESCRIPTOR_RANGE srvMainTable;
	srvMainTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 0);
	slotMainRootParameter[0].InitAsDescriptorTable(1, &srvMainTable, D3D12_SHADER_VISIBILITY_PIXEL);

	CD3DX12_DESCRIPTOR_RANGE samplerTable;
	samplerTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);
	slotMainRootParameter[1].InitAsDescriptorTable(1, &samplerTable, D3D12_SHADER_VISIBILITY_PIXEL);

	pRootSignSecondPass = CreateRootSignature(slotMainRootParameter, _countof(slotMainRootParameter));

	CD3DX12_ROOT_PARAMETER slotRootParameter[4];
	slotRootParameter[0].InitAsConstantBufferView(0);
	slotRootParameter[1].InitAsConstantBufferView(1);
	
	CD3DX12_DESCRIPTOR_RANGE srvTable;
	srvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
	slotRootParameter[2].InitAsDescriptorTable(1, &srvTable, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[3].InitAsDescriptorTable(1, &samplerTable, D3D12_SHADER_VISIBILITY_PIXEL);
	
	pRootSignFirstPass = CreateRootSignature(slotRootParameter, _countof(slotRootParameter));

	wrl::ComPtr<ID3DBlob> pVSByteCode;
	wrl::ComPtr<ID3DBlob> pPSByteCode;
	wrl::ComPtr<ID3DBlob> pVSMainByteCode;
	wrl::ComPtr<ID3DBlob> pPSMainByteCode;
	FDW::Shader::GenerateBytecode(L"shaders/DeferredFirstPass.hlsl", nullptr, "VS", "vs_5_1", pVSByteCode);
	FDW::Shader::GenerateBytecode(L"shaders/DeferredFirstPass.hlsl", nullptr, "PS", "ps_5_1", pPSByteCode);
	FDW::Shader::GenerateBytecode(L"shaders/DeferredSecondPass.hlsl", nullptr, "VS", "vs_5_1", pVSMainByteCode);
	FDW::Shader::GenerateBytecode(L"shaders/DeferredSecondPass.hlsl", nullptr, "PS", "ps_5_1", pPSMainByteCode);

	CD3DX12_RASTERIZER_DESC rasterizerDesc(D3D12_DEFAULT);
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
	rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
	
	DXGI_FORMAT rtvFormats[]{ DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT,DXGI_FORMAT_R32G32B32A32_FLOAT };
	pso = CreatePSO(pRootSignFirstPass->GetRootSignature(), layoutDesc.data(), layoutDesc.size(), 3, rtvFormats, DXGI_FORMAT_D24_UNORM_S8_UINT,
		UINT_MAX, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, pVSByteCode.Get(), pPSByteCode.Get(), nullptr, nullptr, nullptr,
		rasterizerDesc);

	rtvFormats[0] = GetMainRTVFormat();
	mainpso = CreatePSO(pRootSignSecondPass->GetRootSignature(), layoutDesc.data(), layoutDesc.size(), 1, rtvFormats, DXGI_FORMAT_D24_UNORM_S8_UINT,
		UINT_MAX, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, pVSMainByteCode.Get(), pPSMainByteCode.Get(), nullptr, nullptr, nullptr,
		rasterizerDesc);


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

	pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	
	///////////////////////
	// USER RTV DRAW

	rtvPos->StartDraw(pCommandList.Get());
	rtvBase->StartDraw(pCommandList.Get());
	rtvNormals->StartDraw(pCommandList.Get());
	
	pCommandList->ClearDepthStencilView(dsvPack->GetResult()->GetCPUDescriptorHandle(0), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	pCommandList->ClearRenderTargetView(rtvPack->GetResult()->GetCPUDescriptorHandle(0), COLOR, 0, nullptr);
	pCommandList->ClearRenderTargetView(rtvPack->GetResult()->GetCPUDescriptorHandle(1), COLOR, 0, nullptr);
	pCommandList->ClearRenderTargetView(rtvPack->GetResult()->GetCPUDescriptorHandle(2), COLOR, 0, nullptr);
	pCommandList->OMSetRenderTargets(3, &FDW::keep(rtvPack->GetResult()->GetCPUDescriptorHandle(0)), true, &FDW::keep(dsvPack->GetResult()->GetCPUDescriptorHandle(0)));

	pCommandList->SetPipelineState(pso->GetPSO());
	pCommandList->SetGraphicsRootSignature(pRootSignFirstPass->GetRootSignature());

	pCommandList->SetGraphicsRootConstantBufferView(0, pConstantUploadBuffer->GetGPULocation(0));

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

		pCommandList->SetGraphicsRootConstantBufferView(1, pMaterialConstantBuffer->GetGPULocation(std::get<4>(scene->GetObjectParameters(ind))));
		
		UINT indexSize = std::get<2>(scene->GetObjectParameters(ind));
		UINT indexstart = std::get<3>(scene->GetObjectParameters(ind));
		UINT vertexStart = std::get<1>(scene->GetObjectParameters(ind));

		pCommandList->DrawIndexedInstanced(indexSize, 1, indexstart, vertexStart, 0);
	}
	
	rtvPos->EndDraw(pCommandList.Get());
	rtvBase->EndDraw(pCommandList.Get());
	rtvNormals->EndDraw(pCommandList.Get());

	///////////////////////
	// MAIN RTV DRAW
	pCommandList->OMSetRenderTargets(1, &FDW::keep(GetCurrBackBufferView()), true, &FDW::keep(GetDepthStencilView()));
	pCommandList->SetPipelineState(mainpso->GetPSO());
	pCommandList->SetGraphicsRootSignature(pRootSignSecondPass->GetRootSignature());

	ID3D12DescriptorHeap* dcHeaps[] =
	{
		srvPackRTV->GetResult()->GetDescriptorPtr(),
		samplerPack->GetResult()->GetDescriptorPtr()
	};
	pCommandList->SetDescriptorHeaps(_countof(dcHeaps), dcHeaps);
	pCommandList->SetGraphicsRootDescriptorTable(0, srvPackRTV->GetResult()->GetGPUDescriptorHandle(0));
	pCommandList->SetGraphicsRootDescriptorTable(1, samplerPack->GetResult()->GetGPUDescriptorHandle(0));
	pCommandList->IASetVertexBuffers(0, 1, screenRes->GetVertexBufferView());
	pCommandList->IASetIndexBuffer(screenRes->GetIndexBufferView());
	pCommandList->DrawIndexedInstanced(std::get<2>(screenRes->GetObjectParameters(0)), 1, std::get<3>(screenRes->GetObjectParameters(0)), std::get<1>(screenRes->GetObjectParameters(0)), 0);


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
		eye = dx::XMVectorAdd(eye, dx::XMVectorScale(cameraDirection, 100.0f * 0.016f));
	}
	if (wParam == 'S')
	{
		eye = dx::XMVectorSubtract(eye, dx::XMVectorScale(cameraDirection, 100.0f * 0.016f));
	}
	if (wParam == 'D')
	{
		eye = dx::XMVectorAdd(eye, dx::XMVectorScale(rightDirection, 100.0f * 0.016f));
	}
	if (wParam == 'A')
	{
		eye = dx::XMVectorSubtract(eye, dx::XMVectorScale(rightDirection, 100.0f * 0.016f));
	}
}

