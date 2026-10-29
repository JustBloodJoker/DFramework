#pragma once

#include <D3DFramework/GraphicUtilites/ResourcePacker.h>
#include <D3DFramework/GraphicUtilites/StructuredBuffer.h>
#include <MainRenderer/GlobalRenderThreadManager.h>
#include <ShadowData/NeuralShadowData.h>
#include <System/Shadows/ShadowRenderServices.h>
#include <atomic>
#include <memory>
#include <vector>

class NeuralShadowBaselinePass {
    public:
    explicit NeuralShadowBaselinePass(IShadowRenderServices &services);
    ~NeuralShadowBaselinePass() = default;

    public:
    void Initialize();
    void EnsureResourcesAllocated();
    bool AreResourcesAllocated() const;
    void SetInputResources(FD3DW::FResource *shadowAtlas, FD3DW::FResource *normalBuffer, ID3D12Device *device);
    std::shared_ptr<FD3DW::ExecutionHandle> PrepareAndResolve(std::vector<std::shared_ptr<FD3DW::ExecutionHandle>> syncs,
                                                              bool activeShadowWork);
    public:
    D3D12_GPU_VIRTUAL_ADDRESS GetVisibilityGPULocation() const;
    D3D12_GPU_VIRTUAL_ADDRESS GetValidityGPULocation() const;
    D3D12_GPU_VIRTUAL_ADDRESS GetOutputHeaderGPULocation() const;

    protected:
    IShadowRenderServices *m_pServices = nullptr;
    void RebuildInputDescriptors(ID3D12Device *device);

    protected:
    std::unique_ptr<FD3DW::UploadBuffer<FDW_SD::Baseline::NSBaselineDispatchParams>> m_pParamsBuffer;
    std::unique_ptr<FD3DW::StructuredBuffer> m_pPixelFeatures;
    std::unique_ptr<FD3DW::StructuredBuffer> m_pLightFeatures;
    std::unique_ptr<FD3DW::StructuredBuffer> m_pPreparedValidity;
    std::unique_ptr<FD3DW::StructuredBuffer> m_pVisibility;
    std::unique_ptr<FD3DW::StructuredBuffer> m_pValidity;
    std::unique_ptr<FD3DW::StructuredBuffer> m_pOutputHeader;
    std::unique_ptr<FD3DW::SRV_UAVPacker> m_pInputPack;

    FD3DW::FResource *m_pShadowAtlas = nullptr;
    FD3DW::FResource *m_pNormalBuffer = nullptr;
};
