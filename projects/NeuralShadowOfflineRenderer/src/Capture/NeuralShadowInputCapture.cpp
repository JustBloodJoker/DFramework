#include <pch.h>

#include <Capture/NeuralShadowInputCapture.h>
#include <Config/LightingConfig.h>
#include <Config/PipelineRegistry.h>
#include <Config/RenderFormats.h>
#include <Config/RendererLimits.h>
#include <Config/ShaderBindings.h>
#include <Config/ShadowConfig.h>
#include <Config/RendererResources.h>
#include <Renderer/PSOManager.h>

FD3DW::FResource *NeuralShadowInputCapture::GetBlockerValues() const {
    return m_pBlockerValues.get();
}

FD3DW::FResource *NeuralShadowInputCapture::GetInputValidity() const {
    return m_pInputValidity.get();
}

bool NeuralShadowInputCapture::Initialize(ID3D12Device *device, std::string &error) {
    if (!device) {
        error = "neural-shadow input capture requires a D3D12 device";
        return false;
    }

    if (!PSOManager::GetInstance()->GetPSOObjectAs<FD3DW::ComputePipelineObject>(PSOType::NeuralShadowInputCapture)) {
        error = "neural-shadow input-capture pipeline is unavailable";
        return false;
    }

    m_pDevice = device;
    m_pParams = FD3DW::UploadBuffer<Params>::CreateConstantBuffer(device, 1u);
    
    m_pBlockerValues = FD3DW::StructuredBuffer::CreateStructuredBuffer<std::uint32_t>(
        device,
        NSOfflineConfig::NeuralShadows::InputCaptureWorkCapacity * NSOfflineConfig::NeuralShadows::InputCaptureValuesPerWork / 2u,
        false,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    
    m_pInputValidity = FD3DW::StructuredBuffer::CreateStructuredBuffer<std::uint32_t>(
        device,
        NSOfflineConfig::NeuralShadows::InputCaptureWorkCapacity * NSOfflineConfig::NeuralShadows::InputCaptureValuesPerWork / 4u,
        false,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    
    m_pInputs = std::make_unique<FD3DW::SRV_UAVPacker>(
        GetCBV_SRV_UAVDescriptorSize(device),
        2u,
        0u,
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        device);
    
    m_pInputs->AddNullResource(0u, device);
    m_pInputs->AddNullResource(1u, device);
    
    return true;
}

bool NeuralShadowInputCapture::Record(ID3D12GraphicsCommandList *list, const Inputs &inputs, std::string &error) {
    if (!list || !m_pDevice || !m_pParams || !m_pBlockerValues || !m_pInputValidity || !m_pInputs || !inputs.WorkItems ||
        !inputs.WorkHeader || !inputs.ShadowMetadata || !inputs.BlockerDistanceAtlas || !inputs.BlockerValidityAtlas ||
        inputs.ScreenWidth == 0u || inputs.ScreenHeight == 0u) {
        error = "neural-shadow input capture received incomplete live resources";
        return false;
    }
    
    const auto blockerDesc = inputs.BlockerDistanceAtlas->GetResource()->GetDesc();
    if (blockerDesc.Width > UINT_MAX || blockerDesc.Height == 0u) {
        error = "neural-shadow blocker atlas dimensions are invalid";
        return false;
    }
    
    Params params{};
    params.ScreenWidth = inputs.ScreenWidth;
    params.ScreenHeight = inputs.ScreenHeight;
    params.AtlasWidth = static_cast<std::uint32_t>(blockerDesc.Width);
    params.AtlasHeight = blockerDesc.Height;
    params.ShadowMetaCount = inputs.ShadowMetadataCount;
    
    m_pParams->CpyData(0u, params);
    
    m_pInputs->AddResource(inputs.BlockerDistanceAtlas->GetResource(), D3D12_SRV_DIMENSION_TEXTURE2D, 0u, m_pDevice);
    m_pInputs->AddResource(inputs.BlockerValidityAtlas->GetResource(), DXGI_FORMAT_R8_UINT, D3D12_SRV_DIMENSION_TEXTURE2D, 1u, m_pDevice);

    inputs.WorkItems->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    inputs.WorkHeader->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    inputs.ShadowMetadata->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    inputs.BlockerDistanceAtlas->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    inputs.BlockerValidityAtlas->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    
    m_pBlockerValues->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    m_pInputValidity->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    auto* pipeline = PSOManager::GetInstance()->GetPSOObject(PSOType::NeuralShadowInputCapture);
    if (!pipeline) {
        error = "neural-shadow input-capture pipeline is unavailable";
        return false;
    }

    pipeline->Bind(list);
    ID3D12DescriptorHeap *heaps[] = {m_pInputs->GetResult()->GetDescriptorPtr()};
    list->SetDescriptorHeaps(_countof(heaps), heaps);
    
    list->SetComputeRootShaderResourceView(NEURAL_SHADOW_INPUT_CAPTURE_WORK_ITEMS_SRV_POS_IN_ROOT_SIG, inputs.WorkItems->GetResource()->GetGPUVirtualAddress());
    list->SetComputeRootShaderResourceView(NEURAL_SHADOW_INPUT_CAPTURE_WORK_HEADER_SRV_POS_IN_ROOT_SIG, inputs.WorkHeader->GetResource()->GetGPUVirtualAddress());
    list->SetComputeRootShaderResourceView(NEURAL_SHADOW_INPUT_CAPTURE_METADATA_SRV_POS_IN_ROOT_SIG, inputs.ShadowMetadata->GetResource()->GetGPUVirtualAddress());
    list->SetComputeRootDescriptorTable(NEURAL_SHADOW_INPUT_CAPTURE_ATLASES_TABLE_POS_IN_ROOT_SIG, m_pInputs->GetResult()->GetGPUDescriptorHandle(0u));
    list->SetComputeRootUnorderedAccessView(NEURAL_SHADOW_INPUT_CAPTURE_BLOCKERS_UAV_POS_IN_ROOT_SIG, m_pBlockerValues->GetResource()->GetGPUVirtualAddress());
    list->SetComputeRootUnorderedAccessView(NEURAL_SHADOW_INPUT_CAPTURE_VALIDITY_UAV_POS_IN_ROOT_SIG, m_pInputValidity->GetResource()->GetGPUVirtualAddress());
    list->SetComputeRootConstantBufferView(NEURAL_SHADOW_INPUT_CAPTURE_PARAMS_CBV_POS_IN_ROOT_SIG, m_pParams->GetGPULocation(0u));
    
    list->Dispatch(NSOfflineConfig::NeuralShadows::InputCaptureWorkCapacity, 1u, 1u);
    
    m_pBlockerValues->ResourceUAVBarrier(list);
    m_pInputValidity->ResourceUAVBarrier(list);
    
    m_pBlockerValues->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    m_pInputValidity->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    
    return true;
}
