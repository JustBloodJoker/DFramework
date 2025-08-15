#include "DynamicSRV_UAVPacker.h"

namespace FD3DW {

    DynamicSRV_UAVPacker::DynamicSRV_UAVPacker(UINT descriptorSize, UINT descriptorsCount, UINT NodeMask, D3D12_DESCRIPTOR_HEAP_FLAGS flags, ID3D12Device* pDevice) : SRV_UAVPacker(descriptorSize, descriptorsCount, NodeMask, flags, pDevice) {}

    size_t DynamicSRV_UAVPacker::AddResourceDynamic(ID3D12Resource* resource, D3D12_SRV_DIMENSION dimension, ID3D12Device* pDevice) {
        auto index = AllocateIndex(pDevice);
        SRV_UAVPacker::AddResource(resource, dimension, index, pDevice);
        return index;
    }

    size_t DynamicSRV_UAVPacker::AddResourceDynamic(const UAVResourceDesc& desc, ID3D12Device* pDevice) {
        auto index = AllocateIndex(pDevice);
        SRV_UAVPacker::AddResource(desc, index, pDevice);
        return index;
    }

    void DynamicSRV_UAVPacker::RemoveResource(size_t index, ID3D12Device* pDevice)
    {
        if (index < m_uDescriptorCount)
        {
            SRV_UAVPacker::AddNullResource(index, pDevice);
            m_qFreeIndices.push(index);
        }
    }

    size_t DynamicSRV_UAVPacker::AllocateIndex(ID3D12Device* device)
    {
        if (!m_qFreeIndices.empty())
        {
            size_t idx = m_qFreeIndices.front();
            m_qFreeIndices.pop();
            return idx;
        }
     
        if (m_uCurrentIndex >= m_uDescriptorCount)
        {
            ResizeHeap(m_uDescriptorCount * 2, device);
        }

        return m_uCurrentIndex++;
    }




}