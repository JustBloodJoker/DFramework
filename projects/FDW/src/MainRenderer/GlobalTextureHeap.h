#pragma once

#include <pch.h>
#include <D3DFramework/GraphicUtilites/DynamicSRV_UAVPacker.h>
#include <WinWindow/Utils/CreativeSingleton.h>
#include <D3DFramework/GraphicUtilites/FResource.h>

class GlobalTextureHeap : virtual public FD3DW::DynamicSRV_UAVPacker,
	virtual public FDWWIN::CreativeSingleton<GlobalTextureHeap> {

public:
	GlobalTextureHeap();
	GlobalTextureHeap(UINT descriptorSize, UINT descriptorsCount, UINT NodeMask, D3D12_DESCRIPTOR_HEAP_FLAGS flags, ID3D12Device* pDevice);

	void Init(ID3D12Device* device, UINT descriptorsCount = 128, UINT nodeMask = 0);

	size_t AddTexture(std::shared_ptr<FD3DW::FResource> resource, ID3D12Device* device);
	void RemoveTexture(FD3DW::FResource* resource, ID3D12Device* device);
	void CleanupExpired(ID3D12Device* device);
	size_t GetIndex(FD3DW::FResource* resource) const;
private:

	struct ResourceEntry
	{
		std::weak_ptr<FD3DW::FResource> Resource;
		size_t Index = std::numeric_limits<size_t>::max();
	};

	using IndicesMapType = std::unordered_map<FD3DW::FResource*, ResourceEntry>;


private:

	size_t InvalidIndex() const;
	void CleanupExpired_NoLock(ID3D12Device* device);

private:
	void RemoveTexture(size_t idx, ID3D12Device* device);

private:

	mutable std::mutex m_xMutex;
	IndicesMapType m_mIndices;
};