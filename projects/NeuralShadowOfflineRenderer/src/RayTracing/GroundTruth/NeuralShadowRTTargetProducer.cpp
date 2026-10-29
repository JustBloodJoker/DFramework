#include <pch.h>

#include <RayTracing/GroundTruth/NeuralShadowRTTargetProducer.h>

#include <Config/LightingConfig.h>
#include <Config/PipelineRegistry.h>
#include <Config/RenderFormats.h>
#include <Config/RendererLimits.h>
#include <Config/ShaderBindings.h>
#include <Config/ShadowConfig.h>
#include <Renderer/GlobalTextureHeap.h>
#include <Renderer/PSOManager.h>
#include <Config/RendererResources.h>
#include <ShadowData/NeuralShadowData.h>

namespace FDW_SD::GroundTruth {

std::uint32_t NeuralShadowRTTargetProducer::GetGeometryInvalidReason(const NSRTGeometryCoverageState &geometry) {
    
    if (geometry.TLASComplete == 0u || geometry.MissingBLASInstances != 0u)
    {
        return static_cast<std::uint32_t>(FDW_SD::Output::NSOutputValidityReason::MissingAccelerationStructure);
    }

    if (geometry.ActiveTriangleInstances == 0u || geometry.ConservativeAlphaOrOpacityInstances != 0u || geometry.ProceduralInstances != 0u)
    {
        return static_cast<std::uint32_t>(FDW_SD::Output::NSOutputValidityReason::UnsupportedGeometryCoverage);
    }
    
    return static_cast<std::uint32_t>(FDW_SD::Output::NSOutputValidityReason::Valid);
}

bool NeuralShadowRTTargetProducer::HasAllInputs(const NSRTTargetRecordInputs &inputs) {
    return inputs.TLAS && inputs.Lights && inputs.WorkItems && inputs.WorkHeader && inputs.PixelFeatures && inputs.Depth && inputs.Normal &&
           inputs.Visibility && inputs.Validity && inputs.OutputHeader && inputs.GeometryInstances && inputs.GenerationId != 0u &&
           inputs.OutputWidth != 0u && inputs.OutputHeight != 0u;
}

NeuralShadowRTTargetProducer::NeuralShadowRTTargetProducer(ID3D12Device5 *dxrDevice, ID3D12Device *device) : m_pDevice(device) {
    
    auto *pso = PSOManager::GetInstance()->GetPSOObjectAs<FD3DW::RTPipelineObject>(PSOType::NeuralShadowRTTarget);
    
    m_pSBT = std::make_unique<FD3DW::RTShaderBindingTable>(pso);
    m_pSBT->InitSBT(dxrDevice);
    
    m_pParams = FD3DW::UploadBuffer<NSRTTargetDispatchParams>::CreateConstantBuffer(
        device, NSOfflineConfig::NeuralShadows::RTTargetParameterRecordCount);
    
    m_pGBufferPack = std::make_unique<FD3DW::SRV_UAVPacker>(GetCBV_SRV_UAVDescriptorSize(device),
                                                            2u + NSOfflineConfig::NeuralShadows::RTTargetGeometryTextureDescriptors, 0u,
                                                            D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, device);
    
    m_pGBufferPack->AddNullResource(0u, device);
    m_pGBufferPack->AddNullResource(1u, device);
    
    for (std::uint32_t index = 0u; index < NSOfflineConfig::NeuralShadows::RTTargetGeometryTextureDescriptors; ++index)
    {
        m_pGBufferPack->AddNullResource(2u + index, device);
    }
}

bool NeuralShadowRTTargetProducer::Record(ID3D12GraphicsCommandList4 *list, const NSRTTargetRecordInputs &inputs) {
    if (!list || !HasAllInputs(inputs)) return false;
    
    auto settings = ShadowLightParameters::SanitizeQualitySettings(inputs.QualitySettings);
    
    if (settings.Mode != NSRTQualityMode::OfflineGroundTruth || settings.DenoisingPolicy != NSRTDenoisingPolicy::Disabled ||
        inputs.GenerationId != settings.OfflineGenerationId) {
        return false;
    }

    auto quality = ShadowLightParameters::BuildQualityMetadata(settings, inputs.GenerationId, NSRTOutputDomain::ScreenResolutionNSOutput);
    quality.GeometryCoverageFlags = inputs.Geometry.Flags;
    quality.UnsupportedGeometryInstanceCount = inputs.Geometry.ConservativeAlphaOrOpacityInstances + inputs.Geometry.ProceduralInstances;
    quality.MissingAccelerationStructureInstanceCount = inputs.Geometry.MissingBLASInstances;
    
    if (!IsGroundTruthGeometryEligible(inputs.Geometry)) {
        quality.IsCanonicalGroundTruthPreset = 0u;
    }

    m_pGBufferPack->AddResource(inputs.Depth->GetResource(), DXGI_FORMAT_R24_UNORM_X8_TYPELESS, D3D12_SRV_DIMENSION_TEXTURE2D, 0u,m_pDevice);
    m_pGBufferPack->AddResource(inputs.Normal->GetResource(), D3D12_SRV_DIMENSION_TEXTURE2D, 1u, m_pDevice);
    
    auto &globalTextures = GlobalTextureHeap::GetInstance()->GetResult();
    
    if (!globalTextures) return false;
    
    m_pDevice->CopyDescriptorsSimple(NSOfflineConfig::NeuralShadows::RTTargetGeometryTextureDescriptors,
                                     m_pGBufferPack->GetResult()->GetCPUDescriptorHandle(2u), globalTextures->GetCPUDescriptorHandle(0u),
                                     D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    inputs.Lights->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    inputs.WorkItems->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    inputs.WorkHeader->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    inputs.PixelFeatures->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    inputs.Depth->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    inputs.Normal->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    inputs.Visibility->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    inputs.Validity->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    inputs.OutputHeader->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    inputs.GeometryInstances->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    auto *pso = PSOManager::GetInstance()->GetPSOObject(PSOType::NeuralShadowRTTarget);
    pso->Bind(list);
    ID3D12DescriptorHeap *heaps[] = {m_pGBufferPack->GetResult()->GetDescriptorPtr()};
    list->SetDescriptorHeaps(_countof(heaps), heaps);
    
    list->SetComputeRootShaderResourceView(NEURAL_SHADOW_RT_TARGET_TLAS_SRV_POS_IN_ROOT_SIG, inputs.TLAS->GetGPUVirtualAddress());
    list->SetComputeRootShaderResourceView(NEURAL_SHADOW_RT_TARGET_LIGHTS_SRV_POS_IN_ROOT_SIG, inputs.Lights->GetResource()->GetGPUVirtualAddress());
    list->SetComputeRootShaderResourceView(NEURAL_SHADOW_RT_TARGET_WORK_ITEMS_SRV_POS_IN_ROOT_SIG,inputs.WorkItems->GetResource()->GetGPUVirtualAddress());
    list->SetComputeRootShaderResourceView(NEURAL_SHADOW_RT_TARGET_WORK_HEADER_SRV_POS_IN_ROOT_SIG,inputs.WorkHeader->GetResource()->GetGPUVirtualAddress());
    list->SetComputeRootDescriptorTable(NEURAL_SHADOW_RT_TARGET_GBUFFERS_TABLE_POS_IN_ROOT_SIG, m_pGBufferPack->GetResult()->GetGPUDescriptorHandle(0u));
    list->SetComputeRootDescriptorTable(NEURAL_SHADOW_RT_TARGET_GEOMETRY_TEXTURES_TABLE_POS_IN_ROOT_SIG, m_pGBufferPack->GetResult()->GetGPUDescriptorHandle(2u));
    list->SetComputeRootShaderResourceView(NEURAL_SHADOW_RT_TARGET_GEOMETRY_INSTANCES_SRV_POS_IN_ROOT_SIG, inputs.GeometryInstances->GetResource()->GetGPUVirtualAddress());
    
    const auto emptyAddress = GetEmptyStructuredBufferGPUVirtualAddress();
    list->SetComputeRootShaderResourceView(NEURAL_SHADOW_RT_TARGET_GEOMETRY_INDICES_SRV_POS_IN_ROOT_SIG, inputs.GeometryIndexBuffer ? inputs.GeometryIndexBuffer->GetGPUVirtualAddress() : emptyAddress);
   
    list->SetComputeRootShaderResourceView(NEURAL_SHADOW_RT_TARGET_GEOMETRY_VERTICES_SRV_POS_IN_ROOT_SIG,
                                           inputs.GeometryVertexBuffer ? inputs.GeometryVertexBuffer->GetGPUVirtualAddress() : emptyAddress);
    
    list->SetComputeRootShaderResourceView(NEURAL_SHADOW_RT_TARGET_PIXEL_FEATURES_SRV_POS_IN_ROOT_SIG, inputs.PixelFeatures->GetResource()->GetGPUVirtualAddress());
    list->SetComputeRootUnorderedAccessView(NEURAL_SHADOW_RT_TARGET_VISIBILITY_UAV_POS_IN_ROOT_SIG, inputs.Visibility->GetResource()->GetGPUVirtualAddress());
    list->SetComputeRootUnorderedAccessView(NEURAL_SHADOW_RT_TARGET_VALIDITY_UAV_POS_IN_ROOT_SIG, inputs.Validity->GetResource()->GetGPUVirtualAddress());
    list->SetComputeRootUnorderedAccessView(NEURAL_SHADOW_RT_TARGET_HEADER_UAV_POS_IN_ROOT_SIG, inputs.OutputHeader->GetResource()->GetGPUVirtualAddress());

    NSRTTargetDispatchParams common{};
    common.InverseViewProjectionMatrix = inputs.InverseViewProjectionMatrix;
    common.Quality = quality;
    common.LightCount = inputs.LightCount;
    common.GeometryInvalidReason = GetGeometryInvalidReason(inputs.Geometry);
    common.OutputWidth = inputs.OutputWidth;
    common.OutputHeight = inputs.OutputHeight;
    common.LayoutVersion = FDW_SD::Output::OUTPUT_LAYOUT_VERSION;
    common.PassIndex = NSOfflineConfig::NeuralShadows::RTTargetInitializePass;
   
    m_pParams->CpyData(0u, common);
   
    list->SetComputeRootConstantBufferView(NEURAL_SHADOW_RT_TARGET_PARAMS_CBV_POS_IN_ROOT_SIG, m_pParams->GetGPULocation(0u));
    list->DispatchRays(m_pSBT->GetDispatchRaysDesc(std::max(inputs.LightCount, 1u), 1u, 1u));
    
    inputs.OutputHeader->ResourceUAVBarrier(list);

    for (std::uint32_t batch = 0u; batch < NSOfflineConfig::NeuralShadows::RTTargetDispatchBatchCount; ++batch) {
       
        auto params = common;
        params.PassIndex = NSOfflineConfig::NeuralShadows::RTTargetTracePass;
        params.WorkItemBase = batch * NSOfflineConfig::NeuralShadows::RTTargetDispatchBatchCapacity;
        params.DispatchWorkItemCount = std::min(NSOfflineConfig::NeuralShadows::RTTargetDispatchBatchCapacity, FDW_SD::Work::MAX_WORK_ITEMS - params.WorkItemBase);
        
        m_pParams->CpyData(batch + 1u, params);
       
        list->SetComputeRootConstantBufferView(NEURAL_SHADOW_RT_TARGET_PARAMS_CBV_POS_IN_ROOT_SIG, m_pParams->GetGPULocation(batch + 1u));
        list->DispatchRays(m_pSBT->GetDispatchRaysDesc(4u, 16u, params.DispatchWorkItemCount));
    }
    
    inputs.Visibility->ResourceUAVBarrier(list);
    inputs.Validity->ResourceUAVBarrier(list);
    
    common.PassIndex = NSOfflineConfig::NeuralShadows::RTTargetFinalizePass;
    
    m_pParams->CpyData(NSOfflineConfig::NeuralShadows::RTTargetParameterRecordCount - 1u, common);
    list->SetComputeRootConstantBufferView(NEURAL_SHADOW_RT_TARGET_PARAMS_CBV_POS_IN_ROOT_SIG,
                                           m_pParams->GetGPULocation(NSOfflineConfig::NeuralShadows::RTTargetParameterRecordCount - 1u));
    list->DispatchRays(m_pSBT->GetDispatchRaysDesc(1u, 1u, 1u));
    
    inputs.OutputHeader->ResourceUAVBarrier(list);

    inputs.Visibility->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    inputs.Validity->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    inputs.OutputHeader->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    inputs.GeometryInstances->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    inputs.WorkItems->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    inputs.WorkHeader->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    inputs.PixelFeatures->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    inputs.Lights->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    inputs.Depth->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    inputs.Normal->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    
    return true;
}

} // namespace FDW_SD::GroundTruth
