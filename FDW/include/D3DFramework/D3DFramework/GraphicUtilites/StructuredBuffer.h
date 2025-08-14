#pragma once

#include "../pch.h"
#include "FResource.h"

namespace FD3DW {

class StructuredBuffer : public FResource {

public:
	template<typename T>
	static std::unique_ptr<StructuredBuffer> CreateStructuredBuffer(ID3D12Device* pDevice, const UINT count, bool isDynamicScaled) {
		return std::make_unique<StructuredBuffer>(pDevice, count, isDynamicScaled, UINT( sizeof(T) ));
	}

public:
	StructuredBuffer(ID3D12Device* pDevice, const UINT count, bool isDynamicScaled, UINT elemSizeInBytes);
	virtual ~StructuredBuffer() = default;


public:
	void UploadData(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, const void* pData, UINT num, D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	void ShrinkToFit(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList);

private:
	using FResource::UploadData;

private:
	void RecreateBuffer(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, UINT newCapacity, bool preserveData);

private:
	UINT m_uSize;
	UINT m_uCapacity;
	UINT m_iElemSizeInBytes;
	bool m_bIsDynamicScaled;

};

}
