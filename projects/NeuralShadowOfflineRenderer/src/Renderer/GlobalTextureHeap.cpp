#include "pch.h"

#include <Renderer/GlobalTextureHeap.h>
#include <Config/LightingConfig.h>
#include <Config/PipelineRegistry.h>
#include <Config/RendererLimits.h>
#include <Config/RenderFormats.h>
#include <Config/ShaderBindings.h>
#include <Config/ShadowConfig.h>
#include <Config/RendererResources.h>

GlobalTextureHeap::GlobalTextureHeap() : FD3DW::DynamicSRV_UAVPacker(0, 0, 0, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, nullptr) {}

GlobalTextureHeap::GlobalTextureHeap(UINT descriptorSize, UINT descriptorsCount, UINT nodeMask, D3D12_DESCRIPTOR_HEAP_FLAGS flags, ID3D12Device* pDevice) : DynamicSRV_UAVPacker(descriptorSize, descriptorsCount, nodeMask, flags, pDevice) {}

void GlobalTextureHeap::Init(ID3D12Device* device, UINT descriptorsCount, UINT nodeMask) {
    auto descriptorSize = GetCBV_SRV_UAVDescriptorSize(device);
    InitBufferDescriptorHeap(
        descriptorSize,
        descriptorsCount,
        nodeMask,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        device);

    for (auto i = 0u; i < descriptorsCount; ++i) {
        AddNullResource(i, device);
    }
}

size_t GlobalTextureHeap::AddTexture(std::shared_ptr<FD3DW::FResource> resource, ID3D12Device* device) {
    if (!resource) {
        return InvalidIndex();
    }

    std::lock_guard<std::mutex> lock(m_xMutex);

    CleanupExpired_NoLock(device);

    auto raw = resource.get();
    auto it = m_mIndices.find(raw);
    if (it != m_mIndices.end()) {
        return it->second.Index;
    }

    auto index = AddResourceDynamic(resource->GetResource(), D3D12_SRV_DIMENSION_TEXTURE2D, device);

    ResourceEntry entry;
    entry.Resource = resource;
    entry.Index = index;

    m_mIndices.emplace(raw, std::move(entry));
    return index;
}

void GlobalTextureHeap::CleanupExpired_NoLock(ID3D12Device* device) {
    for (auto it = m_mIndices.begin(); it != m_mIndices.end(); ) {
        if (it->second.Resource.expired()) {
            RemoveResource(it->second.Index, device);
            it = m_mIndices.erase(it);
        }
        else {
            ++it;
        }
    }
}

size_t GlobalTextureHeap::InvalidIndex() const {
    return std::numeric_limits<size_t>::max();
}
