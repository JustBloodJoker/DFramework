#pragma once

#include "../pch.h"
#include "FResource.h"

namespace FD3DW {

class StructuredBuffer : public FResource {

public:
	template<typename T>
	static std::unique_ptr<StructuredBuffer> CreateStructuredBuffer(ID3D12Device* pDevice, const UINT count, bool isDynamicScaled, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE, D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT) {
		return std::make_unique<StructuredBuffer>(pDevice, count, isDynamicScaled, UINT( sizeof(T) ), flags, heapType);
	}

	static std::unique_ptr<StructuredBuffer> CreateStructuredBuffer(ID3D12Device* pDevice, const UINT sizeInBytes, const UINT elemSize, bool isDynamicScaled, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE, D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT)  {
		auto elemCounts = CalculateElementsCount(sizeInBytes, elemSize);
		return std::make_unique<StructuredBuffer>(pDevice, elemCounts, isDynamicScaled, elemSize, flags, heapType);
	}
	
	static UINT CalculateElementsCount(UINT sizeInBytes, UINT elemSize);
public:
	StructuredBuffer(ID3D12Device* pDevice, const UINT count, bool isDynamicScaled, UINT elemSizeInBytes, D3D12_RESOURCE_FLAGS flags, D3D12_HEAP_TYPE heapType);
	virtual ~StructuredBuffer() = default;


public:
	void UploadDataNoBarrier(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, const void* pData, UINT num);
	void UploadData(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, const void* pData, UINT num, D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	void UploadRegion(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, const void* pData, UINT elementIndex, UINT numElements, D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	void ReserveSize(ID3D12Device* pDevice, UINT newCapacity);
	void ReserveSize(ID3D12Device* pDevice, UINT sizeInBytes, UINT elemSize);
	void ShrinkToFit(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList);
	void Clear(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, D3D12_RESOURCE_STATES state);

	UINT GetCapacity() const;
	UINT GetSize() const;

private:
	using FResource::UploadDataRegion;
	using FResource::UploadData;

private:
	void ProcessResizing(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, UINT num);
	void RecreateBuffer(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, UINT newCapacity, bool preserveData);

private:
	UINT m_uSize;
	UINT m_uCapacity;
	UINT m_iElemSizeInBytes;
	bool m_bIsDynamicScaled;
	D3D12_RESOURCE_FLAGS m_xResourceFlags;
	D3D12_HEAP_TYPE m_xHeapType;

};

}
