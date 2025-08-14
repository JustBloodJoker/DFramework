#include "StructuredBuffer.h"

namespace FD3DW {

StructuredBuffer::StructuredBuffer(ID3D12Device* pDevice, const UINT count, bool isDynamicScaled, UINT elemSizeInBytes) :
	FResource(
		pDevice,
		1u,
		DXGI_FORMAT_UNKNOWN,
		count* elemSizeInBytes,
		1,
		DXGI_SAMPLE_DESC({ 1, 0 }),
		D3D12_RESOURCE_DIMENSION_BUFFER,
		D3D12_RESOURCE_FLAG_NONE,
		D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
		D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
		&keep(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)),
		1) 
{
	m_bIsDynamicScaled = isDynamicScaled;
	m_uSize = count;
	m_uCapacity = count;
	m_iElemSizeInBytes = elemSizeInBytes;
}

void StructuredBuffer::UploadData(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, const void* pData, UINT num, D3D12_RESOURCE_STATES state) {

	if (num > m_uCapacity) {
		
		if (m_bIsDynamicScaled) {
			UINT newCapacity = std::max(static_cast<UINT>(m_uCapacity * 1.5f), num);
			RecreateBuffer(pDevice, pCommandList, newCapacity, false);

		}
		else {
			SAFE_ASSERT(false, "StructuredBuffer overflow. Enable dynamic scaling or check input size.");
		}

	}

	m_uSize = num;
	FResource::UploadData(pDevice, pCommandList, pData, state, false);


}

void StructuredBuffer::ShrinkToFit(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList) {
	if (m_uSize < m_uCapacity) RecreateBuffer(pDevice, pCommandList, m_uSize, true);
}

void StructuredBuffer::RecreateBuffer(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, UINT newCapacity, bool preserveData) {
	const UINT newSizeInBytes = newCapacity * m_iElemSizeInBytes;

	std::vector<uint8_t> oldData;
	if (m_uSize > 0 && preserveData) {
		oldData = GetData(pDevice, pCommandList);
	}
	
	FResource::CreateTextureBuffer(
		pDevice,
		1u,
		DXGI_FORMAT_UNKNOWN,
		newCapacity * m_iElemSizeInBytes,
		1,
		DXGI_SAMPLE_DESC({ 1, 0 }),
		D3D12_RESOURCE_DIMENSION_BUFFER,
		D3D12_RESOURCE_FLAG_NONE,
		D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
		D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
		&keep(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)),
		1
	);

	m_uCapacity = newCapacity;

	if (preserveData && !oldData.empty()) {
		FResource::UploadData(pDevice, pCommandList, oldData.data(), GetCurrentState(), false);
	}
}


}