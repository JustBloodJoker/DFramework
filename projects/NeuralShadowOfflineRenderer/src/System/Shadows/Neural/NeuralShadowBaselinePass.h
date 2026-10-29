#pragma once

#include <atomic>
#include <memory>
#include <vector>
#include <Renderer/GlobalRenderThreadManager.h>
#include <ShadowData/NeuralShadowData.h>
#include <System/Shadows/ShadowRenderServices.h>
#include <D3DFramework/GraphicUtilites/ResourcePacker.h>
#include <D3DFramework/GraphicUtilites/StructuredBuffer.h>

class NeuralShadowBaselinePass {
public:
	explicit NeuralShadowBaselinePass(IShadowRenderServices& services);
	~NeuralShadowBaselinePass() = default;

	void EnsureResourcesAllocated();
	void SetInputResources(FD3DW::FResource* shadowAtlas, FD3DW::FResource* normalBuffer, ID3D12Device* device);
	std::shared_ptr<FD3DW::ExecutionHandle> PrepareAndResolve(std::vector<std::shared_ptr<FD3DW::ExecutionHandle>> syncs);

	FD3DW::FResource* GetVisibilityResource() const;
	FD3DW::FResource* GetValidityResource() const;
	FD3DW::FResource* GetOutputHeaderResource() const;
	FD3DW::FResource* GetPixelFeaturesResource() const;
	FD3DW::FResource* GetLightFeaturesResource() const;

protected:
	bool AreResourcesAllocated() const;
	void RebuildInputDescriptors(ID3D12Device* device);

	IShadowRenderServices* m_pServices = nullptr;

	std::unique_ptr<FD3DW::UploadBuffer<FDW_SD::Baseline::NSBaselineDispatchParams>> m_pParamsBuffer;
	std::unique_ptr<FD3DW::StructuredBuffer> m_pPixelFeatures;
	std::unique_ptr<FD3DW::StructuredBuffer> m_pLightFeatures;
	std::unique_ptr<FD3DW::StructuredBuffer> m_pVisibility;
	std::unique_ptr<FD3DW::StructuredBuffer> m_pValidity;
	std::unique_ptr<FD3DW::StructuredBuffer> m_pOutputHeader;
	std::unique_ptr<FD3DW::SRV_UAVPacker> m_pInputPack;
	
	FD3DW::FResource* m_pShadowAtlas = nullptr;
	FD3DW::FResource* m_pNormalBuffer = nullptr;
};
