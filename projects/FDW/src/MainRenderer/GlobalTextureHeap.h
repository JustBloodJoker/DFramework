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

	struct WeakPtrHash {
		size_t operator()(const std::weak_ptr<FD3DW::FResource>& w) const noexcept;
	};

	struct WeakPtrEqual {
		bool operator()(const std::weak_ptr<FD3DW::FResource>& a, const std::weak_ptr<FD3DW::FResource>& b) const noexcept;
	};

	using IndicesMapType = std::unordered_map<std::weak_ptr<FD3DW::FResource>, size_t, WeakPtrHash, WeakPtrEqual>;


private:

	void RemoveTexture(size_t idx, ID3D12Device* device);
	IndicesMapType::const_iterator FindByRawPtr(FD3DW::FResource* resource) const;

private:

	IndicesMapType m_mIndices;
};