#include <pch.h>

#include <Config/NeuralShadowConfig.h>
#include <Config/RenderFormats.h>
#include <Config/ShaderBindings.h>
#include <MainRenderer/PSOManager.h>
#include <System/Shadows/Neural/NeuralShadowBaselinePass.h>

NeuralShadowBaselinePass::NeuralShadowBaselinePass(IShadowRenderServices &services) : m_pServices(&services) {}

void NeuralShadowBaselinePass::Initialize() {
    // Heavy output/feature allocation remains deferred.
}

void NeuralShadowBaselinePass::EnsureResourcesAllocated() {
    if (AreResourcesAllocated()) return;

    auto *device = m_pServices->GetDevice();

    m_pParamsBuffer = FD3DW::UploadBuffer<FDW_SD::Baseline::NSBaselineDispatchParams>::CreateConstantBuffer(device, 1u);
    m_pPixelFeatures = FD3DW::StructuredBuffer::CreateStructuredBuffer<std::uint32_t>(device, FDW_SD::Baseline::PIXEL_FEATURE_PACKED_WORD_COUNT, false,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    m_pLightFeatures = FD3DW::StructuredBuffer::CreateStructuredBuffer<std::uint32_t>(device, FDW_SD::Baseline::LIGHT_FEATURE_PACKED_WORD_COUNT, false,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    m_pPreparedValidity = FD3DW::StructuredBuffer::CreateStructuredBuffer<std::uint32_t>(device, FDW_SD::Baseline::PREPARED_VALIDITY_PACKED_WORD_COUNT, false,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    m_pVisibility = FD3DW::StructuredBuffer::CreateStructuredBuffer<std::uint32_t>(device, FDW_SD::Baseline::VISIBILITY_PACKED_WORD_COUNT, false,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    m_pValidity = FD3DW::StructuredBuffer::CreateStructuredBuffer<std::uint32_t>(device, FDW_SD::Baseline::VALIDITY_PACKED_WORD_COUNT, false, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    m_pOutputHeader = FD3DW::StructuredBuffer::CreateStructuredBuffer<FDW_SD::Output::NSOutputHeader>(device, 1u, false, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    RebuildInputDescriptors(device);
}

bool NeuralShadowBaselinePass::AreResourcesAllocated() const {
    return m_pParamsBuffer && m_pPixelFeatures && m_pLightFeatures && m_pPreparedValidity && m_pVisibility && m_pValidity && m_pOutputHeader && m_pInputPack;
}

void NeuralShadowBaselinePass::SetInputResources(FD3DW::FResource *shadowAtlas, FD3DW::FResource *normalBuffer, ID3D12Device *device) {
    m_pShadowAtlas = shadowAtlas;
    m_pNormalBuffer = normalBuffer;
    if (m_pParamsBuffer) RebuildInputDescriptors(device);
}

void NeuralShadowBaselinePass::RebuildInputDescriptors(ID3D12Device *device) {
    SAFE_ASSERT(m_pShadowAtlas, "NeuralShadowBaselinePass: shadow atlas is not bound");
    SAFE_ASSERT(m_pNormalBuffer, "NeuralShadowBaselinePass: normal G-buffer is not bound");

    m_pInputPack = std::make_unique<FD3DW::SRV_UAVPacker>(GetCBV_SRV_UAVDescriptorSize(device), 3u, 0u, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, device);
    m_pInputPack->AddResource(m_pShadowAtlas->GetResource(), D3D12_SRV_DIMENSION_TEXTURE2D, FDWConfig::NeuralShadows::BaselineAtlasDescriptor, device);
    m_pInputPack->AddNullResource(FDWConfig::NeuralShadows::BaselineDepthDescriptor, device);
    m_pInputPack->AddResource(m_pNormalBuffer->GetResource(), D3D12_SRV_DIMENSION_TEXTURE2D, FDWConfig::NeuralShadows::BaselineNormalDescriptor, device);
}

std::shared_ptr<FD3DW::ExecutionHandle> NeuralShadowBaselinePass::PrepareAndResolve(std::vector<std::shared_ptr<FD3DW::ExecutionHandle>> syncs, bool activeShadowWork) {
    EnsureResourcesAllocated();
    auto *depthBuffer = m_pServices->GetCurrentDepthResource();
    auto recipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_DIRECT, [this, activeShadowWork, depthBuffer](ID3D12GraphicsCommandList *list) {

        SAFE_ASSERT(m_pInputPack, "NeuralShadowBaselinePass: input descriptors are not bound");
        SAFE_ASSERT(m_pShadowAtlas, "NeuralShadowBaselinePass: shadow atlas is not bound");
        SAFE_ASSERT(m_pNormalBuffer, "NeuralShadowBaselinePass: normal G-buffer is not bound");
        SAFE_ASSERT(depthBuffer, "NeuralShadowBaselinePass: depth buffer is not available");

        FDW_SD::Baseline::NSBaselineDispatchParams params{};
        params.ScreenWidth = static_cast<UINT>(std::max(1, m_pServices->GetSceneRenderWidth()));
        params.ScreenHeight = static_cast<UINT>(std::max(1, m_pServices->GetSceneRenderHeight()));

        const auto atlasDesc = m_pShadowAtlas->GetResource()->GetDesc();
        params.AtlasWidth = static_cast<UINT>(atlasDesc.Width);
        params.AtlasHeight = atlasDesc.Height;
        params.LightCount = activeShadowWork ? static_cast<UINT>(std::max(0, m_pServices->GetLightsCount())) : 0u;
        params.ShadowMetaCount = activeShadowWork ? m_pServices->GetShadowMetadataCount() : 0u;
        params.CameraNear = m_pServices->GetCameraNear();
        params.CameraFar = m_pServices->GetCameraFar();
        params.InverseViewProjectionMatrix = dx::XMMatrixInverse(nullptr, dx::XMMatrixTranspose(m_pServices->GetJitteredViewProjectionMatrix()));

        m_pParamsBuffer->CpyData(0u, params);

        auto *pso = PSOManager::GetInstance()->GetPSOObject(PSOType::NeuralShadowBaseline);
        m_pInputPack->AddResource(depthBuffer, FDWConfig::NeuralShadows::BaselineDepthDescriptor, m_pServices->GetDevice());

        if (activeShadowWork) {
            m_pShadowAtlas->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            m_pNormalBuffer->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            depthBuffer->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            m_pServices->PrepareShadowMetadataForCompute(list);
            m_pServices->GetLightsBuffer()->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        }

        m_pVisibility->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        m_pValidity->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        m_pOutputHeader->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    #define BIND_BASELINE()                                                                                                                                                               \
                {                                                                                                                                                                       \
                    pso->Bind(list);                                                                                                                                                    \
                    ID3D12DescriptorHeap* heaps[] = { m_pInputPack->GetResult()->GetDescriptorPtr() };                                                                                  \
                    list->SetDescriptorHeaps(_countof(heaps), heaps);                                                                                                                   \
                                                                                                                                                                                        \
                    list->SetComputeRootShaderResourceView(NEURAL_SHADOW_BASELINE_WORK_ITEMS_SRV_POS_IN_ROOT_SIG, m_pServices->GetNeuralWorkItemsAddress());                               \
                    list->SetComputeRootShaderResourceView(NEURAL_SHADOW_BASELINE_WORK_HEADER_SRV_POS_IN_ROOT_SIG, m_pServices->GetNeuralWorkHeaderAddress());                             \
                    list->SetComputeRootShaderResourceView(NEURAL_SHADOW_BASELINE_LIGHTS_SRV_POS_IN_ROOT_SIG, m_pServices->GetLightsBufferAddress());                                     \
                    list->SetComputeRootShaderResourceView(NEURAL_SHADOW_BASELINE_METADATA_SRV_POS_IN_ROOT_SIG, m_pServices->GetShadowMetadataAddress());                                 \
                    list->SetComputeRootDescriptorTable(NEURAL_SHADOW_BASELINE_INPUTS_TABLE_POS_IN_ROOT_SIG, m_pInputPack->GetResult()->GetGPUDescriptorHandle(0u));                        \
                    list->SetComputeRootUnorderedAccessView(NEURAL_SHADOW_BASELINE_PIXEL_FEATURES_UAV_POS_IN_ROOT_SIG, m_pPixelFeatures->GetResource()->GetGPUVirtualAddress());           \
                    list->SetComputeRootUnorderedAccessView(NEURAL_SHADOW_BASELINE_LIGHT_FEATURES_UAV_POS_IN_ROOT_SIG, m_pLightFeatures->GetResource()->GetGPUVirtualAddress());           \
                    list->SetComputeRootUnorderedAccessView(NEURAL_SHADOW_BASELINE_VISIBILITY_UAV_POS_IN_ROOT_SIG, m_pVisibility->GetResource()->GetGPUVirtualAddress());                 \
                    list->SetComputeRootUnorderedAccessView(NEURAL_SHADOW_BASELINE_VALIDITY_UAV_POS_IN_ROOT_SIG, m_pValidity->GetResource()->GetGPUVirtualAddress());                     \
                                                                                                                                                                                        \
                    list->SetComputeRootUnorderedAccessView(NEURAL_SHADOW_BASELINE_OUTPUT_HEADER_UAV_POS_IN_ROOT_SIG, m_pOutputHeader->GetResource()->GetGPUVirtualAddress());             \
                    list->SetComputeRootConstantBufferView(NEURAL_SHADOW_BASELINE_PARAMS_CBV_POS_IN_ROOT_SIG, m_pParamsBuffer->GetGPULocation(0u));                                   \
                    list->SetComputeRootUnorderedAccessView(NEURAL_SHADOW_BASELINE_PREPARED_VALIDITY_UAV_POS_IN_ROOT_SIG, m_pPreparedValidity->GetResource()->GetGPUVirtualAddress());     \
                }

        UINT passConstants[] = {static_cast<UINT>(FDW_SD::Baseline::NSBaselinePass::Initialize), 0u,
                                static_cast<UINT>(FDW_SD::Output::NSOutputProducer::DeterministicBaseline)};

        BIND_BASELINE();

        list->SetComputeRoot32BitConstants(NEURAL_SHADOW_BASELINE_PASS_CONSTANT_POS_IN_ROOT_SIG, _countof(passConstants), passConstants, 0u);
        list->Dispatch(1u, 1u, 1u);
        m_pOutputHeader->ResourceUAVBarrier(list);

        if (activeShadowWork) {

            constexpr UINT featureThreadCount = FDW_SD::Baseline::BATCH_CAPACITY * FDW_SD::Features::PIXELS_PER_PATCH / 2u;
            constexpr UINT resolveThreadCount = FDW_SD::Baseline::BATCH_CAPACITY * FDW_SD::Features::PIXELS_PER_PATCH / 4u;

            for (UINT batchIndex = 0u; batchIndex < FDW_SD::Baseline::BATCH_COUNT; ++batchIndex) {
                m_pPixelFeatures->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                m_pLightFeatures->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                m_pPreparedValidity->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

                BIND_BASELINE();

                passConstants[0] = static_cast<UINT>(FDW_SD::Baseline::NSBaselinePass::PrepareFeatures);
                passConstants[1] = batchIndex * FDW_SD::Baseline::BATCH_CAPACITY;
                list->SetComputeRoot32BitConstants(NEURAL_SHADOW_BASELINE_PASS_CONSTANT_POS_IN_ROOT_SIG, _countof(passConstants), passConstants, 0u);
                list->Dispatch((featureThreadCount + FDWConfig::NeuralShadows::BaselineThreadCountX - 1u) / FDWConfig::NeuralShadows::BaselineThreadCountX, 1u, 1u);

                m_pPixelFeatures->ResourceUAVBarrier(list);
                m_pLightFeatures->ResourceUAVBarrier(list);
                m_pPreparedValidity->ResourceUAVBarrier(list);
                m_pValidity->ResourceUAVBarrier(list);

                passConstants[0] = static_cast<UINT>(FDW_SD::Baseline::NSBaselinePass::ResolveNearest);
                list->SetComputeRoot32BitConstants(NEURAL_SHADOW_BASELINE_PASS_CONSTANT_POS_IN_ROOT_SIG, _countof(passConstants), passConstants, 0u);
                list->Dispatch((resolveThreadCount + FDWConfig::NeuralShadows::BaselineThreadCountX - 1u) / FDWConfig::NeuralShadows::BaselineThreadCountX, 1u, 1u);

                m_pPixelFeatures->ResourceUAVBarrier(list);
                m_pVisibility->ResourceUAVBarrier(list);
                m_pValidity->ResourceUAVBarrier(list);
            }
        }

        BIND_BASELINE();

        m_pOutputHeader->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        passConstants[0] = static_cast<UINT>(FDW_SD::Baseline::NSBaselinePass::Finalize);
        passConstants[1] = 0u;

        list->SetComputeRoot32BitConstants(NEURAL_SHADOW_BASELINE_PASS_CONSTANT_POS_IN_ROOT_SIG, _countof(passConstants), passConstants, 0u);
        list->Dispatch(1u, 1u, 1u);

        m_pOutputHeader->ResourceUAVBarrier(list);
        m_pPixelFeatures->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        m_pLightFeatures->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        m_pPreparedValidity->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        m_pVisibility->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        m_pValidity->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        m_pOutputHeader->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

        if (activeShadowWork) {
            m_pShadowAtlas->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            m_pNormalBuffer->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            depthBuffer->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            m_pServices->PrepareShadowMetadataForPixel(list);
            m_pServices->GetLightsBuffer()->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        }
    });

    #undef BIND_BASELINE

    auto handle = GlobalRenderThreadManager::GetInstance()->Submit(recipe, syncs, true);
    return handle;
}

D3D12_GPU_VIRTUAL_ADDRESS NeuralShadowBaselinePass::GetVisibilityGPULocation() const {
    return m_pVisibility ? m_pVisibility->GetResource()->GetGPUVirtualAddress() : 0u;
}

D3D12_GPU_VIRTUAL_ADDRESS NeuralShadowBaselinePass::GetValidityGPULocation() const {
    return m_pValidity ? m_pValidity->GetResource()->GetGPUVirtualAddress() : 0u;
}

D3D12_GPU_VIRTUAL_ADDRESS NeuralShadowBaselinePass::GetOutputHeaderGPULocation() const {
    return m_pOutputHeader ? m_pOutputHeader->GetResource()->GetGPUVirtualAddress() : 0u;
}
