#include <System/TAASystem.h>
#include <MainRenderer/MainRenderer.h>
#include <MainRenderer/PSOManager.h>

void TAASystem::AfterConstruction() {
	auto wndSettings = m_pOwner->GetMainWNDSettings();
	auto device = m_pOwner->GetDevice();

	m_pDataBuffer = FD3DW::UploadBuffer<TAASystemData>::CreateConstantBuffer(device, 1);

	m_pRTVPack = std::make_unique<FD3DW::RTVPacker>(GetRTVDescriptorSize(device), 2u, 0, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, device);
	m_pSRVPack = FD3DW::SRV_UAVPacker::CreatePack(GetCBV_SRV_UAVDescriptorSize(device), 6u, 0, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, device);

	for (auto& historyBuffer : m_pHistoryBuffers){
		historyBuffer = std::make_unique<FD3DW::RenderTarget>(device, GetForwardRenderPassFormat(), D3D12_RTV_DIMENSION_TEXTURE2D, 1, wndSettings.Width, wndSettings.Height, DXGI_SAMPLE_DESC({ 1, 0 }));

		m_pRTVPack->PushResource(historyBuffer->GetRTVResource(), historyBuffer->GetRTVDesc(), device);
		m_pSRVPack->PushResource(device, historyBuffer->GetRTVResource(), D3D12_SRV_DIMENSION_TEXTURE2D);
	}

	m_pSRVPack->AddNullResource(TAA_SCENE_SHADING_RESULT_SRV_POS_IN_HEAP, device);
	m_pSRVPack->AddNullResource(TAA_MOTION_GBUFFER_SRV_POS_IN_HEAP, device);

	auto recipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_DIRECT, [this](ID3D12GraphicsCommandList* list) {
		m_pScreen = std::make_unique<FD3DW::Rectangle>(m_pOwner->GetDevice(), list);
		m_pSceneVBV_IBV = std::make_unique<FD3DW::ObjectVertexIndexDataCreator<FD3DW::VertexFrameWork>>();
		m_pSceneVBV_IBV->Create(m_pOwner->GetDevice(), list, m_pScreen->GetVertices(), m_pScreen->GetIndices());

		for (auto i = 0; i < 2; ++i) {
			list->ClearRenderTargetView(m_pRTVPack->GetResult()->GetCPUDescriptorHandle(i), m_pOwner->GetClearColor(), 0, nullptr);
		}
	});

	GlobalRenderThreadManager::GetInstance()->Submit(recipe);
}

void TAASystem::SetGBufferResources(FD3DW::FResource* sceneShading, FD3DW::FResource* motion, FD3DW::DepthStencilView* dsv1, FD3DW::DepthStencilView* dsv2) {
	auto device = m_pOwner->GetDevice();
	m_pSRVPack->AddResource(sceneShading->GetResource(), D3D12_SRV_DIMENSION_TEXTURE2D, TAA_SCENE_SHADING_RESULT_SRV_POS_IN_HEAP, device);
	m_pSRVPack->AddResource(motion->GetResource(), D3D12_SRV_DIMENSION_TEXTURE2D, TAA_MOTION_GBUFFER_SRV_POS_IN_HEAP, device);
	m_pSRVPack->AddResource(dsv1, TAA_DEPTH_1_BUFFER_SRV_POS_IN_HEAP, device);
	m_pSRVPack->AddResource(dsv2, TAA_DEPTH_2_BUFFER_SRV_POS_IN_HEAP, device);
}

std::shared_ptr<FD3DW::ExecutionHandle> TAASystem::ProcessTAABufferCollection(std::vector<std::shared_ptr<FD3DW::ExecutionHandle>> sync) {
	if (!m_bIsTAAEnabled) return nullptr;

	auto recipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_DIRECT, [this](ID3D12GraphicsCommandList* list) {
		auto frameIdx = m_pOwner->GetFrameIndex();
		auto writeIndex = frameIdx % 2;
		auto readIndex = (frameIdx + 1) % 2;
		
		auto writeRTV = m_pHistoryBuffers[writeIndex].get();
		auto readRTV = m_pHistoryBuffers[readIndex].get();
		
		TAASystemData data;
		data.CurrentReaderIndex = readIndex;
		data.CurrentWriterIndex = writeIndex;
		data.CurrentDepthBufferIndex = m_pOwner->GetCurrentDSVIndex();
		data.BlendWeight = 0.1f;
		data.CurrentJitterOffset = m_pOwner->GetCurrentJitterOffset();
		data.PrevJitterOffset = m_pOwner->GetPrevJitterOffset();
		m_pDataBuffer->CpyData(0, data);
		
		list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		auto objParams = m_pScreen->GetObjectParameters(0);

		list->RSSetScissorRects(1, &FD3DW::keep(m_pOwner->GetMainRect()));
		list->RSSetViewports(1, &FD3DW::keep(m_pOwner->GetMainViewPort()));

		list->IASetVertexBuffers(0, 1, m_pSceneVBV_IBV->GetVertexBufferView());
		list->IASetIndexBuffer(m_pSceneVBV_IBV->GetIndexBufferView());

		ID3D12DescriptorHeap* heaps[] = { m_pSRVPack->GetResult()->GetDescriptorPtr() };
		list->SetDescriptorHeaps(_countof(heaps), heaps);

		writeRTV->StartDraw(list);

		list->OMSetRenderTargets(1, &FD3DW::keep(m_pRTVPack->GetResult()->GetCPUDescriptorHandle(writeIndex)), true, nullptr);
		
		PSOManager::GetInstance()->GetPSOObject(PSOType::TAAPass)->Bind(list);
		list->SetGraphicsRootDescriptorTable(TAA_SRV_POS_IN_ROOT_SIG, m_pSRVPack->GetResult()->GetGPUDescriptorHandle(0));
		list->SetGraphicsRootConstantBufferView(TAA_DATA_BUFFER_POS_IN_ROOT_SIG, m_pDataBuffer->GetGPULocation(0));
		list->DrawIndexedInstanced((UINT)objParams.IndicesCount, 1, (UINT)objParams.IndicesOffset, (UINT)objParams.VerticesOffset, 0);

		writeRTV->EndDraw(list);
	});
	return GlobalRenderThreadManager::GetInstance()->Submit(recipe, sync, true);
}

FD3DW::FResource* TAASystem::GetCurrentResultResource() {
	auto frameIdx = m_pOwner->GetFrameIndex();
	auto writeIndex = frameIdx % 2;
	return m_pHistoryBuffers[writeIndex]->GetTexture();
}

bool TAASystem::IsTAAEnabled() {
	return m_bIsTAAEnabled;
}

void TAASystem::EnableTAA(bool b) {
	m_bIsTAAEnabled = b;
}
