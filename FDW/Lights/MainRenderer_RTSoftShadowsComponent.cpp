#include <Lights/MainRenderer_RTSoftShadowsComponent.h>
#include <MainRenderer/MainRenderer.h>
#include <MainRenderer/PSOManager.h>

void MainRenderer_RTSoftShadowsComponent::AfterConstruction() {
    MainRenderer_ShadowsComponent::AfterConstruction();

	auto dxrDevice = m_pOwner->GetDXRDevice();
	auto dxrCommandList = m_pOwner->GetDXRCommandList();
	auto wndSettings = m_pOwner->GetMainWNDSettings();

	auto pso = PSOManager::GetInstance()->GetPSOObjectAs<FD3DW::RTPipelineObject>(PSOType::RTSoftShadowDefaultConfig);
	m_pSoftShadowsSBT = std::make_unique<FD3DW::RTShaderBindingTable>(pso);
	m_pSoftShadowsSBT->InitSBT(dxrDevice);
	
	m_pResultResource = FD3DW::FResource::CreateAnonimTexture(dxrDevice, 1, DXGI_FORMAT_R32G32B32A32_FLOAT, wndSettings.Width, wndSettings.Height, {1,0}, D3D12_RESOURCE_DIMENSION_TEXTURE2D, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_TEXTURE_LAYOUT_UNKNOWN, D3D12_HEAP_FLAG_NONE, &FD3DW::keep(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)),1u);
	m_pResultResource->ResourceBarrierChange(dxrCommandList, 1, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	
	m_pSoftShadowsUAVPacker = std::make_unique<FD3DW::SRV_UAVPacker>(GetCBV_SRV_UAVDescriptorSize(dxrDevice), 3u, 0, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, dxrDevice);
	
	FD3DW::UAVResourceDesc desc;
	desc.Resource = m_pResultResource->GetResource();
	desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	desc.MipSlice = 0;
	desc.PlaneSlice = 0;
	m_pSoftShadowsUAVPacker->AddResource(desc, 0, dxrDevice);
}

bool MainRenderer_RTSoftShadowsComponent::IsCanBeEnabled(MainRenderer* renderer) {
    return renderer->IsRTSupported();
}

void MainRenderer_RTSoftShadowsComponent::BeforeGBufferPass() {}

void MainRenderer_RTSoftShadowsComponent::AfterGBufferPass() {
	auto dxrCommandList = m_pOwner->GetDXRCommandList();
	auto tlas = m_pOwner->GetTLAS(dxrCommandList).pResult;

	if (!tlas) return;

	auto wndSettings = m_pOwner->GetMainWNDSettings();

	PSOManager::GetInstance()->GetPSOObject(PSOType::RTSoftShadowDefaultConfig)->Bind(dxrCommandList);

	dxrCommandList->SetComputeRootShaderResourceView(RT_SOFT_SHADOW_TLAS_BUFFER_POS_IN_ROOT_SIG, tlas->GetGPUVirtualAddress());

	ID3D12DescriptorHeap* r[1] = { m_pSoftShadowsUAVPacker->GetResult()->GetDescriptorPtr() };
	dxrCommandList->SetDescriptorHeaps(ARRAYSIZE(r), r);
	dxrCommandList->SetComputeRootDescriptorTable(RT_SOFT_SHADOW_UAV_SHADOWS_OUT_POS_IN_ROOT_SIG, m_pSoftShadowsUAVPacker->GetResult()->GetGPUDescriptorHandle(0));
	dxrCommandList->SetComputeRootDescriptorTable(RT_SOFT_SHADOW_GBUFFERS_POS_IN_ROOT_SIG, m_pSoftShadowsUAVPacker->GetResult()->GetGPUDescriptorHandle(1));
	
	m_pOwner->BindLightConstantBuffer(RT_SOFT_SHADOW_LIGHTS_HELPER_BUFFER_POS_IN_ROOT_SIG, RT_SOFT_SHADOW_LIGHTS_BUFFER_POS_IN_ROOT_SIG, dxrCommandList, true);

	dxrCommandList->DispatchRays(m_pSoftShadowsSBT->GetDispatchRaysDesc(wndSettings.Width, wndSettings.Height, 1));
}

D3D12_SRV_DIMENSION MainRenderer_RTSoftShadowsComponent::GetSRVResultDimension() {
    return D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;
}

void MainRenderer_RTSoftShadowsComponent::SetGBuffersResources(FD3DW::FResource* worldPos, FD3DW::FResource* normal, ID3D12Device* device) {
	m_pSoftShadowsUAVPacker->AddResource(worldPos->GetResource(), D3D12_SRV_DIMENSION_TEXTURE2D,1, device);
	m_pSoftShadowsUAVPacker->AddResource(normal->GetResource(), D3D12_SRV_DIMENSION_TEXTURE2D,2, device);
}
