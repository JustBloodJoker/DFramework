#include <MainRenderer/GlobalTextureHeap.h>
#include <MainRenderer/GlobalConfig.h>

GlobalTextureHeap::GlobalTextureHeap() : FD3DW::DynamicSRV_UAVPacker(0, 0, 0, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, nullptr) {}

void GlobalTextureHeap::Init(ID3D12Device* device, UINT descriptorsCount, UINT nodeMask)
{
    UINT descriptorSize = GetCBV_SRV_UAVDescriptorSize(device);
    InitBufferDescriptorHeap(descriptorSize, descriptorsCount, nodeMask, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, device);
}

GlobalTextureHeap::GlobalTextureHeap(UINT descriptorSize, UINT descriptorsCount, UINT NodeMask, D3D12_DESCRIPTOR_HEAP_FLAGS flags, ID3D12Device* pDevice) : DynamicSRV_UAVPacker(descriptorSize, descriptorsCount, NodeMask, flags, pDevice) {}

size_t GlobalTextureHeap::AddTexture(std::shared_ptr<FD3DW::FResource> resource, ID3D12Device* device) {
    CleanupExpired(device);

    auto it = FindByRawPtr(resource.get());
    if (it != m_mIndices.end())
        return it->second;

    size_t index = AddResourceDynamic(resource->GetResource(), D3D12_SRV_DIMENSION_TEXTURE2D, device);
    m_mIndices.emplace(resource, index);

    return index;
}

void GlobalTextureHeap::RemoveTexture(FD3DW::FResource* resource, ID3D12Device* device) {
    auto it = FindByRawPtr(resource);
    if (it != m_mIndices.end())
    {
        size_t idx = it->second;
        RemoveResource(idx, device);
        m_mIndices.erase(it);
    }
}

void GlobalTextureHeap::RemoveTexture(size_t idx, ID3D12Device* device)
{
    RemoveResource(idx, device);
}

GlobalTextureHeap::IndicesMapType::const_iterator GlobalTextureHeap::FindByRawPtr(FD3DW::FResource* resource) const {
    for (auto it = m_mIndices.begin(); it != m_mIndices.end(); ++it)
    {
        if (it->first.lock().get() == resource)
            return it;
    }
    return m_mIndices.end();
}

void GlobalTextureHeap::CleanupExpired(ID3D12Device* device) {
    for (auto it = m_mIndices.begin(); it != m_mIndices.end(); )
    {
        if (it->first.expired())
        {
            RemoveTexture(it->second, device);
            it = m_mIndices.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

size_t GlobalTextureHeap::GetIndex(FD3DW::FResource* resource) const {
    auto it = FindByRawPtr(resource);
    if (it != m_mIndices.end())
        return it->second;

    return -1;
}

size_t GlobalTextureHeap::WeakPtrHash::operator()(const std::weak_ptr<FD3DW::FResource>& w) const noexcept
{
    return std::hash<FD3DW::FResource*>()(w.lock().get());
}


bool GlobalTextureHeap::WeakPtrEqual::operator()(const std::weak_ptr<FD3DW::FResource>& a, const std::weak_ptr<FD3DW::FResource>& b) const noexcept
{
    return a.lock().get() == b.lock().get();
}
 ;