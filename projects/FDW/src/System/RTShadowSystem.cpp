#include <System/RTShadowSystem.h>
#include <MainRenderer/MainRenderer.h>
#include <MainRenderer/PSOManager.h>

void RTShadowSystem::AfterConstruction() {
	auto dxrDevice = m_pOwner->GetDXRDevice();
	auto wndSettings = m_pOwner->GetMainWNDSettings();

	m_pFrameBuffer = std::make_unique<FD3DW::UploadBuffer<RTShadowSystemBuffer>>(dxrDevice, 1, true);

	auto pso = PSOManager::GetInstance()->GetPSOObjectAs<FD3DW::RTPipelineObject>(PSOType::RTSoftShadowDefaultConfig);
	m_pSoftShadowsSBT = std::make_unique<FD3DW::RTShaderBindingTable>(pso);
	m_pSoftShadowsSBT->InitSBT(dxrDevice);

	m_pSrcResultResource = FD3DW::FResource::CreateAnonimTexture(dxrDevice, 1, DXGI_FORMAT_R32G32B32A32_FLOAT, wndSettings.Width, wndSettings.Height, { 1,0 }, D3D12_RESOURCE_DIMENSION_TEXTURE2D, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_TEXTURE_LAYOUT_UNKNOWN, D3D12_HEAP_FLAG_NONE, &FD3DW::keep(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)), 1u);

	m_pSoftShadowsUAVPacker = std::make_unique<FD3DW::SRV_UAVPacker>(GetCBV_SRV_UAVDescriptorSize(dxrDevice), 7u, 0, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, dxrDevice);

	FD3DW::UAVResourceDesc desc;
	desc.Resource = m_pSrcResultResource->GetResource();
	desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	desc.MipSlice = 0;
	desc.PlaneSlice = 0;
	m_pSoftShadowsUAVPacker->AddResource(desc, 0, dxrDevice);

	m_vInOutWorldPosAndShadowInfo.push_back(FD3DW::FResource::CreateAnonimTexture(dxrDevice, 1, DXGI_FORMAT_R32G32B32A32_FLOAT, wndSettings.Width, wndSettings.Height, { 1,0 }, D3D12_RESOURCE_DIMENSION_TEXTURE2D, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_TEXTURE_LAYOUT_UNKNOWN, D3D12_HEAP_FLAG_NONE, &FD3DW::keep(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)), 1u));
	m_vInOutWorldPosAndShadowInfo.push_back(FD3DW::FResource::CreateAnonimTexture(dxrDevice, 1, DXGI_FORMAT_R32G32B32A32_FLOAT, wndSettings.Width, wndSettings.Height, { 1,0 }, D3D12_RESOURCE_DIMENSION_TEXTURE2D, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_TEXTURE_LAYOUT_UNKNOWN, D3D12_HEAP_FLAG_NONE, &FD3DW::keep(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)), 1u));


	FD3DW::UAVResourceDesc descInOut1;
	descInOut1.Resource = m_vInOutWorldPosAndShadowInfo[0]->GetResource();
	descInOut1.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	descInOut1.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	descInOut1.MipSlice = 0;
	descInOut1.PlaneSlice = 0;
	m_pSoftShadowsUAVPacker->AddResource(descInOut1, 3, dxrDevice);
	m_pSoftShadowsUAVPacker->AddResource(m_vInOutWorldPosAndShadowInfo[0]->GetResource(), D3D12_SRV_DIMENSION_TEXTURE2D, 4, dxrDevice);

	FD3DW::UAVResourceDesc descInOut2;
	descInOut2.Resource = m_vInOutWorldPosAndShadowInfo[1]->GetResource();
	descInOut2.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	descInOut2.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	descInOut2.MipSlice = 0;
	descInOut2.PlaneSlice = 0;
	m_pSoftShadowsUAVPacker->AddResource(descInOut2, 5, dxrDevice);
	m_pSoftShadowsUAVPacker->AddResource(m_vInOutWorldPosAndShadowInfo[1]->GetResource(), D3D12_SRV_DIMENSION_TEXTURE2D, 6, dxrDevice);

	FD3DW::BilateralParams params = {};
	params.KernelRadius = 7;
	params.SigmaS = 3;
	params.SigmaR = 0.3f;
	m_pBilateralFilter = std::make_unique<FD3DW::BilateralFilter>(dxrDevice, m_pSrcResultResource.get(), params);

	m_xConfig.PrevViewProj = dx::XMMatrixTranspose(m_pOwner->GetCurrentViewMatrix() * m_pOwner->GetCurrentProjectionMatrix());
}

void RTShadowSystem::ProcessNotify(NRenderSystemNotifyType type) {

}

std::shared_ptr<FD3DW::ExecutionHandle> RTShadowSystem::OnStartRenderTick(std::shared_ptr<FD3DW::ExecutionHandle> handle) {
	auto recipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_DIRECT, [this](ID3D12GraphicsCommandList* list) {
		m_iCurrentShadowBufferUsage = (m_iCurrentShadowBufferUsage + 1) % 2;
		RTShadowSystemBuffer buffer = m_xConfig;
		buffer.PrevViewProj = buffer.CurrViewProj;
		buffer.FrameIndex = m_pOwner->GetFrameIndex();
		buffer.CurrViewProj = dx::XMMatrixTranspose(m_pOwner->GetCurrentViewMatrix() * m_pOwner->GetCurrentProjectionMatrix());
		m_pFrameBuffer->CpyData(0, buffer);

		FD3DW::BilateralParams params = m_xConfig;
		m_pBilateralFilter->SetParams(params);
	});
	return GlobalRenderThreadManager::GetInstance()->Submit(recipe, { handle });
}

std::shared_ptr<FD3DW::ExecutionHandle> RTShadowSystem::OnRenderShadowFactors(std::vector<std::shared_ptr<FD3DW::ExecutionHandle>> handle) {
	auto recipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList4>>(D3D12_COMMAND_LIST_TYPE_DIRECT, [this](ID3D12GraphicsCommandList4* list) {
		auto tlas = m_pOwner->GetTLAS().pResult;

		if (!tlas) return;

		auto wndSettings = m_pOwner->GetMainWNDSettings();

		PSOManager::GetInstance()->GetPSOObject(PSOType::RTSoftShadowDefaultConfig)->Bind(list);

		list->SetComputeRootShaderResourceView(RT_SOFT_SHADOW_TLAS_BUFFER_POS_IN_ROOT_SIG, tlas->GetGPUVirtualAddress());

		m_pSrcResultResource->ResourceBarrierChange(list, 1, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		ID3D12DescriptorHeap* r[1] = { m_pSoftShadowsUAVPacker->GetResult()->GetDescriptorPtr() };
		list->SetDescriptorHeaps(ARRAYSIZE(r), r);
		list->SetComputeRootDescriptorTable(RT_SOFT_SHADOW_UAV_SHADOWS_OUT_POS_IN_ROOT_SIG, m_pSoftShadowsUAVPacker->GetResult()->GetGPUDescriptorHandle(0));
		list->SetComputeRootDescriptorTable(RT_SOFT_SHADOW_GBUFFERS_POS_IN_ROOT_SIG, m_pSoftShadowsUAVPacker->GetResult()->GetGPUDescriptorHandle(1));

		list->SetComputeRootConstantBufferView(RT_SOFT_SHADOW_LIGHTS_HELPER_BUFFER_POS_IN_ROOT_SIG, m_pOwner->GetLightBufferConstantBufferAddress());
		list->SetComputeRootShaderResourceView(RT_SOFT_SHADOW_LIGHTS_BUFFER_POS_IN_ROOT_SIG, m_pOwner->GetLightsStructuredBufferAddress());
		list->SetComputeRootShaderResourceView(RT_SOFT_SHADOW_CLUSTERS_BUFFER_POS_IN_ROOT_SIG, m_pOwner->GetClusterStructuredBufferAddress());
		list->SetComputeRootConstantBufferView(RT_SOFT_SHADOW_CLUSTERS_DATA_BUFFER_POS_IN_ROOT_SIG, m_pOwner->GetClusterConstantBufferAddress());
		list->SetComputeRootConstantBufferView(RT_SOFT_SHADOW_FRAME_BUFFER_POS_IN_ROOT_SIG, m_pFrameBuffer->GetGPULocation(0));

		if (m_iCurrentShadowBufferUsage == 1) {
			m_vInOutWorldPosAndShadowInfo[0]->ResourceBarrierChange(list, 1, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			m_vInOutWorldPosAndShadowInfo[1]->ResourceBarrierChange(list, 1, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			list->SetComputeRootDescriptorTable(RT_SOFT_SHADOW_INPUT_PREV_FRAME_SRV_POS_IN_ROOT_SIG, m_pSoftShadowsUAVPacker->GetResult()->GetGPUDescriptorHandle(4));
			list->SetComputeRootDescriptorTable(RT_SOFT_SHADOW_OUTPUT_CURR_FRAME_UAV_POS_IN_ROOT_SIG, m_pSoftShadowsUAVPacker->GetResult()->GetGPUDescriptorHandle(5));
		}
		else {
			m_vInOutWorldPosAndShadowInfo[1]->ResourceBarrierChange(list, 1, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			m_vInOutWorldPosAndShadowInfo[0]->ResourceBarrierChange(list, 1, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			list->SetComputeRootDescriptorTable(RT_SOFT_SHADOW_INPUT_PREV_FRAME_SRV_POS_IN_ROOT_SIG, m_pSoftShadowsUAVPacker->GetResult()->GetGPUDescriptorHandle(6));
			list->SetComputeRootDescriptorTable(RT_SOFT_SHADOW_OUTPUT_CURR_FRAME_UAV_POS_IN_ROOT_SIG, m_pSoftShadowsUAVPacker->GetResult()->GetGPUDescriptorHandle(3));
		}

		list->DispatchRays(m_pSoftShadowsSBT->GetDispatchRaysDesc(wndSettings.Width, wndSettings.Height, 1));

		m_pBilateralFilter->Apply(list);
	});

	return GlobalRenderThreadManager::GetInstance()->Submit(recipe, handle);
}


void RTShadowSystem::SetGBuffersResources(FD3DW::FResource* worldPos, FD3DW::FResource* normal, ID3D12Device* device) {
	m_pSoftShadowsUAVPacker->AddResource(worldPos->GetResource(), D3D12_SRV_DIMENSION_TEXTURE2D, 1, device);
	m_pSoftShadowsUAVPacker->AddResource(normal->GetResource(), D3D12_SRV_DIMENSION_TEXTURE2D, 2, device);
}

void RTShadowSystem::SetConfig(RTShadowSystemConfig config) {
	m_xConfig = config;
}

RTShadowSystemConfig RTShadowSystem::GetConfig() {
	return m_xConfig;
}

FD3DW::FResource* RTShadowSystem::GetResultResource()
{
	return m_pBilateralFilter->GetDstResource();
}

