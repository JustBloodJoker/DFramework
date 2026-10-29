#include <pch.h>

#include <Config/NeuralShadowConfig.h>
#include <Config/ShaderBindings.h>
#include <Renderer/PSOManager.h>
#include <System/ClusteredLightTileData.h>
#include <System/Shadows/Neural/NeuralShadowWorkGenerator.h>

NeuralShadowWorkGenerator::NeuralShadowWorkGenerator(IShadowRenderServices& services)
	: m_pServices(&services) {
}

void NeuralShadowWorkGenerator::EnsureResourcesAllocated() {
	if (AreResourcesAllocated()) return;

	auto* device = m_pServices->GetDevice();
	m_pGenerationParamsBuffer = FD3DW::UploadBuffer<FDW_SD::Work::NSWorkGenerationParams>::CreateConstantBuffer(device, 1u);
	m_pPatchRecords = FD3DW::StructuredBuffer::CreateStructuredBuffer<FDW_SD::Work::NSWorkPatchRecord>(device,FDW_SD::Work::MAX_WORK_ITEMS,false,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	m_pWorkItems = FD3DW::StructuredBuffer::CreateStructuredBuffer<FDW_SD::Work::NSWorkItem>(device,FDW_SD::Work::MAX_WORK_ITEMS,false,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	m_pDispatchHeader = FD3DW::StructuredBuffer::CreateStructuredBuffer<FDW_SD::Work::NSWorkDispatchHeader>(device,1u,false,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
}

bool NeuralShadowWorkGenerator::AreResourcesAllocated() const {
	return m_pGenerationParamsBuffer && m_pPatchRecords && m_pWorkItems && m_pDispatchHeader;
}

std::shared_ptr<FD3DW::ExecutionHandle> NeuralShadowWorkGenerator::GenerateWorkItems(std::vector<std::shared_ptr<FD3DW::ExecutionHandle>> syncs,std::uint64_t generationOverride) {
	EnsureResourcesAllocated();

	const auto screenWidth = static_cast<UINT>(std::max(1, m_pServices->GetSceneRenderWidth()));
	const auto screenHeight = static_cast<UINT>(std::max(1, m_pServices->GetSceneRenderHeight()));
	const auto patchCountX = FDW_SD::Work::ComputePatchCountX(screenWidth);
	const auto patchCountY = FDW_SD::Work::ComputePatchCountY(screenHeight);
	const auto patchCount64 = static_cast<std::uint64_t>(patchCountX) * patchCountY;
	const auto boundedPatchCount = static_cast<UINT>(std::min<std::uint64_t>(patchCount64,FDW_SD::Work::MAX_WORK_ITEMS));
	
	auto generation = generationOverride;
	if (generation == 0u) {
		generation = m_uNextGeneration.fetch_add(1u, std::memory_order_relaxed);

		if (generation == 0u) {
			generation = m_uNextGeneration.fetch_add(1u, std::memory_order_relaxed);
		}
	}

	auto recipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		[this, screenWidth, screenHeight, patchCountX, patchCountY, boundedPatchCount, generation](ID3D12GraphicsCommandList* list) {
			
			FDW_SD::Work::NSWorkGenerationParams params{};
			params.ScreenWidth = screenWidth;
			params.ScreenHeight = screenHeight;
			params.PatchCountX = patchCountX;
			params.PatchCountY = patchCountY;
			params.LightCount = static_cast<UINT>(std::max(0, m_pServices->GetLightsCount()));
			params.ShadowMetaCount = m_pServices->GetShadowMetadataCount();
			params.ActiveShadowWork = 1u;
			params.GenerationLow = static_cast<UINT>(generation);
			params.GenerationHigh = static_cast<UINT>(generation >> 32u);
			params.CoarseGridWidth = NSClusteredLightTiles::GRID_WIDTH;
			params.CoarseGridHeight = NSClusteredLightTiles::GRID_HEIGHT;
			params.CoarseTileCount = NSClusteredLightTiles::TILE_COUNT;
			
			m_pGenerationParamsBuffer->CpyData(0u, params);

			auto* pso = PSOManager::GetInstance()->GetPSOObject(PSOType::NeuralShadowWorkGeneration);
			pso->Bind(list);
			m_pServices->PrepareShadowMetadataForCompute(list);

			m_pPatchRecords->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			m_pWorkItems->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			m_pDispatchHeader->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			m_pPatchRecords->ResourceUAVBarrier(list);

			list->SetComputeRootShaderResourceView(NEURAL_SHADOW_WORK_CLUSTERS_SRV_POS_IN_ROOT_SIG,m_pServices->GetClusterBufferAddress());
			list->SetComputeRootShaderResourceView(NEURAL_SHADOW_WORK_METADATA_SRV_POS_IN_ROOT_SIG,m_pServices->GetShadowMetadataAddress());
			list->SetComputeRootUnorderedAccessView(NEURAL_SHADOW_WORK_PATCH_RECORDS_UAV_POS_IN_ROOT_SIG,m_pPatchRecords->GetResource()->GetGPUVirtualAddress());
			list->SetComputeRootUnorderedAccessView(NEURAL_SHADOW_WORK_ITEMS_UAV_POS_IN_ROOT_SIG,m_pWorkItems->GetResource()->GetGPUVirtualAddress());
			list->SetComputeRootUnorderedAccessView(NEURAL_SHADOW_WORK_HEADER_UAV_POS_IN_ROOT_SIG,m_pDispatchHeader->GetResource()->GetGPUVirtualAddress());
			list->SetComputeRootConstantBufferView(NEURAL_SHADOW_WORK_PARAMS_CBV_POS_IN_ROOT_SIG,m_pGenerationParamsBuffer->GetGPULocation(0u));

			if (boundedPatchCount != 0u) {
				list->SetComputeRoot32BitConstant(NEURAL_SHADOW_WORK_PASS_CONSTANT_POS_IN_ROOT_SIG,NSOfflineConfig::NeuralShadows::WorkCountPass,0u);
				
				list->Dispatch((boundedPatchCount + NSOfflineConfig::NeuralShadows::WorkThreadCountX - 1u) /NSOfflineConfig::NeuralShadows::WorkThreadCountX,1u,1u);
				
				m_pPatchRecords->ResourceUAVBarrier(list);
			}

			list->SetComputeRoot32BitConstant(NEURAL_SHADOW_WORK_PASS_CONSTANT_POS_IN_ROOT_SIG,NSOfflineConfig::NeuralShadows::WorkScanPass,0u);
			
			list->Dispatch(1u, 1u, 1u);
			
			m_pPatchRecords->ResourceUAVBarrier(list);
			m_pDispatchHeader->ResourceUAVBarrier(list);

			if (boundedPatchCount != 0u) {
				list->SetComputeRoot32BitConstant(NEURAL_SHADOW_WORK_PASS_CONSTANT_POS_IN_ROOT_SIG,NSOfflineConfig::NeuralShadows::WorkWritePass,0u);
				
				list->Dispatch((boundedPatchCount + NSOfflineConfig::NeuralShadows::WorkThreadCountX - 1u) /NSOfflineConfig::NeuralShadows::WorkThreadCountX,1u,1u);
				
				m_pWorkItems->ResourceUAVBarrier(list);
				m_pDispatchHeader->ResourceUAVBarrier(list);
			}

			m_pWorkItems->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
			m_pPatchRecords->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
			m_pDispatchHeader->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		});

	auto handle = GlobalRenderThreadManager::GetInstance()->Submit(recipe, syncs, true);
	return handle;
}

D3D12_GPU_VIRTUAL_ADDRESS NeuralShadowWorkGenerator::GetWorkItemsGPULocation() const {
	return m_pWorkItems ? m_pWorkItems->GetResource()->GetGPUVirtualAddress() : 0u;
}

D3D12_GPU_VIRTUAL_ADDRESS NeuralShadowWorkGenerator::GetDispatchHeaderGPULocation() const {
	return m_pDispatchHeader ? m_pDispatchHeader->GetResource()->GetGPUVirtualAddress() : 0u;
}

FD3DW::FResource* NeuralShadowWorkGenerator::GetWorkItemsResource() const {
	return m_pWorkItems.get();
}

FD3DW::FResource* NeuralShadowWorkGenerator::GetDispatchHeaderBufferResource() const {
	return m_pDispatchHeader.get();
}
