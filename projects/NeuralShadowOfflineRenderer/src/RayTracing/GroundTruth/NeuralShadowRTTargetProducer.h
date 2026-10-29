#pragma once

#include <Config/NeuralShadowConfig.h>
#include <D3DFramework/GraphicUtilites/RTShaderBindingTable.h>
#include <D3DFramework/GraphicUtilites/ResourcePacker.h>
#include <D3DFramework/GraphicUtilites/StructuredBuffer.h>
#include <ShadowData/ShadowLightParameters.h>
#include <System/Shadows/RayTracing/RTGeometryCoverage.h>

namespace FDW_SD::GroundTruth {

struct NSRTTargetDispatchParams {
    dx::XMFLOAT4X4 InverseViewProjectionMatrix{};
    
    NSRTQualityMetadata Quality{};
   
    std::uint32_t PassIndex = 0u;
    std::uint32_t WorkItemBase = 0u;
    std::uint32_t DispatchWorkItemCount = 0u;
    std::uint32_t LightCount = 0u;
    std::uint32_t GeometryInvalidReason = 0u;
    std::uint32_t OutputWidth = 0u;
    std::uint32_t OutputHeight = 0u;
    std::uint32_t LayoutVersion = NSOfflineConfig::NeuralShadows::RTTargetDispatchLayout;
};

struct NSRTTargetRecordInputs {
    ID3D12Resource *TLAS = nullptr;
    
    FD3DW::FResource *Lights = nullptr;
    FD3DW::FResource *WorkItems = nullptr;
    FD3DW::FResource *WorkHeader = nullptr;
    FD3DW::FResource *PixelFeatures = nullptr;
    FD3DW::FResource *Depth = nullptr;
    FD3DW::FResource *Normal = nullptr;
    FD3DW::FResource *Visibility = nullptr;
    FD3DW::FResource *Validity = nullptr;
    FD3DW::FResource *OutputHeader = nullptr;
   
    dx::XMFLOAT4X4 InverseViewProjectionMatrix{};
    
    NSRTQualitySettings QualitySettings{};
    NSRTGeometryCoverageState Geometry{};
    
    FD3DW::FResource *GeometryInstances = nullptr;
    
    ID3D12Resource *GeometryIndexBuffer = nullptr;
    ID3D12Resource *GeometryVertexBuffer = nullptr;
    
    std::uint64_t GenerationId = 0u;
    std::uint32_t LightCount = 0u;
    std::uint32_t OutputWidth = 0u;
    std::uint32_t OutputHeight = 0u;
};

// Explicit capture-only recorder. Constructing this object allocates only a small SBT,
// descriptor heap, and constant buffer; target storage is the caller's NS_OUTPUT storage.
class NeuralShadowRTTargetProducer {
  public:
    NeuralShadowRTTargetProducer(ID3D12Device5 *dxrDevice, ID3D12Device *device);
    bool Record(ID3D12GraphicsCommandList4 *list, const NSRTTargetRecordInputs &inputs);

  protected:
    static std::uint32_t GetGeometryInvalidReason(const NSRTGeometryCoverageState &geometry);
    static bool HasAllInputs(const NSRTTargetRecordInputs &inputs);

protected:
    std::unique_ptr<FD3DW::RTShaderBindingTable> m_pSBT;
    std::unique_ptr<FD3DW::UploadBuffer<NSRTTargetDispatchParams>> m_pParams;
    std::unique_ptr<FD3DW::SRV_UAVPacker> m_pGBufferPack;
    
    ID3D12Device *m_pDevice = nullptr;
};

} // namespace FDW_SD::GroundTruth
