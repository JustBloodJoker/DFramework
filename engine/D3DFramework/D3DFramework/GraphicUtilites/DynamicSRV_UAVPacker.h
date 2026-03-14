#pragma once

#include "../pch.h"
#include "ResourcePacker.h"

namespace FD3DW {

    class DynamicSRV_UAVPacker : public SRV_UAVPacker
	{
	public:
		DynamicSRV_UAVPacker(UINT descriptorSize, UINT descriptorsCount,UINT NodeMask, D3D12_DESCRIPTOR_HEAP_FLAGS flags, ID3D12Device* pDevice);
		virtual ~DynamicSRV_UAVPacker() = default;

        size_t AddResourceDynamic(ID3D12Resource* resource, D3D12_SRV_DIMENSION dimension, ID3D12Device* pDevice);
        size_t AddResourceDynamic(const UAVResourceDesc& desc, ID3D12Device* pDevice);
        void RemoveResource(size_t index, ID3D12Device* pDevice);

    private:
        size_t AllocateIndex(ID3D12Device* device);

    private:
        std::queue<size_t> m_qFreeIndices;
	};


}
