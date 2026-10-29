#pragma once

#include <atomic>
#include <memory>
#include <vector>
#include <Renderer/GlobalRenderThreadManager.h>
#include <System/Shadows/ShadowRenderServices.h>
#include <D3DFramework/GraphicUtilites/StructuredBuffer.h>
#include <ShadowData/NeuralShadowData.h>

class NeuralShadowWorkGenerator {
public:
	explicit NeuralShadowWorkGenerator(IShadowRenderServices& services);
	~NeuralShadowWorkGenerator() = default;

	void EnsureResourcesAllocated();
	std::shared_ptr<FD3DW::ExecutionHandle> GenerateWorkItems(std::vector<std::shared_ptr<FD3DW::ExecutionHandle>> syncs,std::uint64_t generationOverride = 0u);

	D3D12_GPU_VIRTUAL_ADDRESS GetWorkItemsGPULocation() const;
	D3D12_GPU_VIRTUAL_ADDRESS GetDispatchHeaderGPULocation() const;
	FD3DW::FResource* GetWorkItemsResource() const;
	FD3DW::FResource* GetDispatchHeaderBufferResource() const;

protected:
	bool AreResourcesAllocated() const;

	IShadowRenderServices* m_pServices = nullptr;

	std::unique_ptr<FD3DW::UploadBuffer<FDW_SD::Work::NSWorkGenerationParams>> m_pGenerationParamsBuffer;
	std::unique_ptr<FD3DW::StructuredBuffer> m_pPatchRecords;
	std::unique_ptr<FD3DW::StructuredBuffer> m_pWorkItems;
	std::unique_ptr<FD3DW::StructuredBuffer> m_pDispatchHeader;
	std::atomic<std::uint64_t> m_uNextGeneration{ 1u };
};
