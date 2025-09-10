#include <System/BloomEffectSystem.h>
#include <MainRenderer/MainRenderer.h>
#include <MainRenderer/PSOManager.h>


void BloomEffectSystem::AfterConstruction() {
	auto wndSettings = m_pOwner->GetMainWNDSettings();
	auto device = m_pOwner->GetDevice();
	m_pBloomRTV = std::make_unique<FD3DW::RenderTarget>(device, GetForwardRenderPassFormat(), D3D12_RTV_DIMENSION_TEXTURE2D, 1, wndSettings.Width, wndSettings.Height, DXGI_SAMPLE_DESC({1, 0}));
	m_pResultRTV = std::make_unique<FD3DW::RenderTarget>(device, GetForwardRenderPassFormat(), D3D12_RTV_DIMENSION_TEXTURE2D, 1, wndSettings.Width, wndSettings.Height, DXGI_SAMPLE_DESC({1, 0}));
	m_pBlurTransitRTV = std::make_unique<FD3DW::RenderTarget>(device, GetForwardRenderPassFormat(), D3D12_RTV_DIMENSION_TEXTURE2D, 1, wndSettings.Width, wndSettings.Height, DXGI_SAMPLE_DESC({1, 0}));

	m_pBloomRTVPack = std::make_unique<FD3DW::RTVPacker>(GetRTVDescriptorSize(device), 3u, 0, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, device);
	m_pBloomRTVPack->AddResource(m_pBloomRTV->GetRTVResource(), m_pBloomRTV->GetRTVDesc(), 0, device);
	m_pBloomRTVPack->AddResource(m_pBlurTransitRTV->GetRTVResource(), m_pBlurTransitRTV->GetRTVDesc(), 1, device);
	m_pBloomRTVPack->AddResource(m_pResultRTV->GetRTVResource(), m_pResultRTV->GetRTVDesc(), 2, device);
	
	m_pBloomSRVPack = FD3DW::SRV_UAVPacker::CreatePack(GetCBV_SRV_UAVDescriptorSize(device), 4u, 0, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, device);
	m_pBloomSRVPack->AddNullResource(0, device);
	m_pBloomSRVPack->AddResource(m_pBloomRTV->GetRTVResource(), D3D12_SRV_DIMENSION_TEXTURE2D, 1u, device);
	m_pBloomSRVPack->AddResource(m_pBlurTransitRTV->GetRTVResource(), D3D12_SRV_DIMENSION_TEXTURE2D, 2u, device);
	m_pBloomSRVPack->AddResource(m_pResultRTV->GetRTVResource(), D3D12_SRV_DIMENSION_TEXTURE2D, 3u, device);

	m_pBrightPassDataBuffer = FD3DW::UploadBuffer<BloomSystemBrightPassData>::CreateConstantBuffer(device, 1);
	m_pBlurParamsBuffer = FD3DW::UploadBuffer<BloomSystemBlurParams>::CreateConstantBuffer(device, 2);
	m_pCompositeDataBuffer = FD3DW::UploadBuffer<BloomSystemCompositeData>::CreateConstantBuffer(device, 1);

	BloomSystemBlurParams xBlur;
	xBlur.TexelSize = dx::XMFLOAT2(1.0f / wndSettings.Width, 1.0f / wndSettings.Height);
	xBlur.Horizontal = 1;
	m_pBlurParamsBuffer->CpyData(0, xBlur);

	BloomSystemBlurParams yBlur;
	yBlur.TexelSize = dx::XMFLOAT2(1.0f / wndSettings.Width, 1.0f / wndSettings.Height);
	yBlur.Horizontal = 0;
	m_pBlurParamsBuffer->CpyData(1, yBlur);

	auto recipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_DIRECT, [this](ID3D12GraphicsCommandList* list) {
		m_pScreen = std::make_unique<FD3DW::Rectangle>(m_pOwner->GetDevice(), list);
		m_pSceneVBV_IBV = std::make_unique<FD3DW::ObjectVertexIndexDataCreator<FD3DW::VertexFrameWork>>();
		m_pSceneVBV_IBV->Create(m_pOwner->GetDevice(), list, m_pScreen->GetVertices(), m_pScreen->GetIndices());
	});

	GlobalRenderThreadManager::GetInstance()->Submit(recipe);
}

std::shared_ptr<FD3DW::ExecutionHandle> BloomEffectSystem::ProcessBloomPass(std::shared_ptr<FD3DW::ExecutionHandle> handle) {
	if (!m_bIsBloomEnabled) return nullptr;
	
	auto recipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_DIRECT, [this](ID3D12GraphicsCommandList* list) {
		if (m_bIsNeedUpdateBrightPassData.exchange(false, std::memory_order_acq_rel)) {
			m_pBrightPassDataBuffer->CpyData(0, m_xBrightPassData);
		}
		if (m_bIsNeedUpdateCompositeData.exchange(false, std::memory_order_acq_rel)) {
			m_pCompositeDataBuffer->CpyData(0, m_xCompositeData);
		}
		
		list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); 
		
		auto objParams = m_pScreen->GetObjectParameters(0);
		
		list->RSSetScissorRects(1, &FD3DW::keep(m_pOwner->GetMainRect()));
		list->RSSetViewports(1, &FD3DW::keep(m_pOwner->GetMainViewPort()));

		list->IASetVertexBuffers(0, 1, m_pSceneVBV_IBV->GetVertexBufferView());
		list->IASetIndexBuffer(m_pSceneVBV_IBV->GetIndexBufferView());
		
		ID3D12DescriptorHeap* heaps[] = { m_pBloomSRVPack->GetResult()->GetDescriptorPtr() };
		list->SetDescriptorHeaps(_countof(heaps), heaps);

		m_pBloomRTV->StartDraw(list);
		list->ClearRenderTargetView(m_pBloomRTVPack->GetResult()->GetCPUDescriptorHandle(0), m_pOwner->GetClearColor(), 0, nullptr);
		list->OMSetRenderTargets(1, &FD3DW::keep(m_pBloomRTVPack->GetResult()->GetCPUDescriptorHandle(0)), true, nullptr);
		PSOManager::GetInstance()->GetPSOObject(PSOType::BloomEffect_BrightPass)->Bind(list);
		list->SetGraphicsRootDescriptorTable(BLOOM_EFFECT_BRIGHT_PASS_SRV_INPUT_POS_IN_ROOT_SIG, m_pBloomSRVPack->GetResult()->GetGPUDescriptorHandle(0));
		list->SetGraphicsRootConstantBufferView(BLOOM_EFFECT_BRIGHT_PASS_BRIGHT_CBV_INPUT_POS_IN_ROOT_SIG, m_pBrightPassDataBuffer->GetGPULocation(0));
		list->DrawIndexedInstanced((UINT)objParams.IndicesCount, 1, (UINT)objParams.IndicesOffset, (UINT)objParams.VerticesOffset, 0);
		m_pBloomRTV->EndDraw(list);

		auto type = m_xBloomBlurType;
		auto isHorizontal = (type & BloomBlurType::Horizontal) == BloomBlurType::Horizontal;
		auto isVertical = (type & BloomBlurType::Vertical) == BloomBlurType::Vertical;
		if (isHorizontal) {
			m_pBlurTransitRTV->StartDraw(list);
			list->ClearRenderTargetView(m_pBloomRTVPack->GetResult()->GetCPUDescriptorHandle(1), m_pOwner->GetClearColor(), 0, nullptr);
			list->OMSetRenderTargets(1, &FD3DW::keep(m_pBloomRTVPack->GetResult()->GetCPUDescriptorHandle(1)), true, nullptr);
			PSOManager::GetInstance()->GetPSOObject(PSOType::BloomEffect_GaussianBlurPass)->Bind(list);
			list->SetGraphicsRootDescriptorTable(BLOOM_EFFECT_GAUSSIAN_BLUR_BLOOM_SRV_POS_IN_ROOT_SIG, m_pBloomSRVPack->GetResult()->GetGPUDescriptorHandle(1));
			list->SetGraphicsRootConstantBufferView(BLOOM_EFFECT_GAUSSIAN_BLUR_PARAMS_CBV_POS_IN_ROOT_SIG, m_pBlurParamsBuffer->GetGPULocation(0));
			list->DrawIndexedInstanced((UINT)objParams.IndicesCount, 1, (UINT)objParams.IndicesOffset, (UINT)objParams.VerticesOffset, 0);
			m_pBlurTransitRTV->EndDraw(list);
		}

		if (isVertical) {
			isHorizontal ? m_pBloomRTV->StartDraw(list) : m_pBlurTransitRTV->StartDraw(list);
			list->ClearRenderTargetView(m_pBloomRTVPack->GetResult()->GetCPUDescriptorHandle(isHorizontal ? 0 : 1), m_pOwner->GetClearColor(), 0, nullptr);
			list->OMSetRenderTargets(1, &FD3DW::keep(m_pBloomRTVPack->GetResult()->GetCPUDescriptorHandle(isHorizontal ? 0 : 1)), true, nullptr);
			PSOManager::GetInstance()->GetPSOObject(PSOType::BloomEffect_GaussianBlurPass)->Bind(list);
			list->SetGraphicsRootDescriptorTable(BLOOM_EFFECT_GAUSSIAN_BLUR_BLOOM_SRV_POS_IN_ROOT_SIG, m_pBloomSRVPack->GetResult()->GetGPUDescriptorHandle(isHorizontal ? 2 : 1));
			list->SetGraphicsRootConstantBufferView(BLOOM_EFFECT_GAUSSIAN_BLUR_PARAMS_CBV_POS_IN_ROOT_SIG, m_pBlurParamsBuffer->GetGPULocation(1));
			list->DrawIndexedInstanced((UINT)objParams.IndicesCount, 1, (UINT)objParams.IndicesOffset, (UINT)objParams.VerticesOffset, 0);
			isHorizontal ? m_pBloomRTV->EndDraw(list) : m_pBlurTransitRTV->StartDraw(list);
		}

		m_pResultRTV->StartDraw(list);
		list->ClearRenderTargetView(m_pBloomRTVPack->GetResult()->GetCPUDescriptorHandle(2), m_pOwner->GetClearColor(), 0, nullptr);
		list->OMSetRenderTargets(1, &FD3DW::keep(m_pBloomRTVPack->GetResult()->GetCPUDescriptorHandle(2)), true, nullptr);
		PSOManager::GetInstance()->GetPSOObject(PSOType::BloomEffect_CompositePass)->Bind(list);
		list->SetGraphicsRootDescriptorTable(BLOOM_EFFECT_COMPOSITE_PASS_SRV_POS_IN_ROOT_SIG, m_pBloomSRVPack->GetResult()->GetGPUDescriptorHandle(0));
		list->SetGraphicsRootDescriptorTable(BLOOM_EFFECT_COMPOSITE_PASS_BLUR_SRV_POS_IN_ROOT_SIG, m_pBloomSRVPack->GetResult()->GetGPUDescriptorHandle(isHorizontal ? 1 : 2));
		list->SetGraphicsRootConstantBufferView(BLOOM_EFFECT_COMPOSITE_PASS_CBV_PARAMS_POS_IN_ROOT_SIG, m_pCompositeDataBuffer->GetGPULocation(0));
		list->DrawIndexedInstanced((UINT)objParams.IndicesCount, 1, (UINT)objParams.IndicesOffset, (UINT)objParams.VerticesOffset, 0);
		m_pResultRTV->EndDraw(list);
	});
	
	return GlobalRenderThreadManager::GetInstance()->Submit(recipe, {handle});
}

void BloomEffectSystem::SetShadingOutputResourceResultAndRect(FD3DW::FResource* shadingRes) {
	m_pBloomSRVPack->AddResource(shadingRes->GetResource(), D3D12_SRV_DIMENSION_TEXTURE2D, 0u, m_pOwner->GetDevice());
}

bool BloomEffectSystem::IsEnabledBloom() {
	return m_bIsBloomEnabled;
}

void BloomEffectSystem::EnableBloom(bool b) {
	m_bIsBloomEnabled = b;
}

ID3D12Resource* BloomEffectSystem::GetResultResource() {
	return m_pResultRTV->GetRTVResource();
}

BloomSystemCompositeData BloomEffectSystem::GetCompositeData() {
	return m_xCompositeData;
}

void BloomEffectSystem::SetCompositeData(BloomSystemCompositeData data) {
	m_xCompositeData = data;
	m_bIsNeedUpdateCompositeData.store(true, std::memory_order_relaxed);
}

BloomSystemBrightPassData BloomEffectSystem::GetBrightPassData() {
	return m_xBrightPassData;
}

void BloomEffectSystem::SetBrightPassData(BloomSystemBrightPassData data) {
	m_xBrightPassData = data;
	m_bIsNeedUpdateBrightPassData.store(true, std::memory_order_relaxed);
}

BloomBlurType BloomEffectSystem::GetBloomBlurType() {
	return m_xBloomBlurType;
}

void BloomEffectSystem::SetBloomBlurType(BloomBlurType blurType) {
	m_xBloomBlurType = blurType;
}
