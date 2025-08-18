#include "StructuredBuffer.h"

namespace FD3DW {

StructuredBuffer::StructuredBuffer(ID3D12Device* pDevice, const UINT count, bool isDynamicScaled, UINT elemSizeInBytes, D3D12_RESOURCE_FLAGS flags, D3D12_HEAP_TYPE heapType) :
	FResource(
		pDevice,
		1u,
		DXGI_FORMAT_UNKNOWN,
		count* elemSizeInBytes,
		1,
		DXGI_SAMPLE_DESC({ 1, 0 }),
		D3D12_RESOURCE_DIMENSION_BUFFER,
		flags,
		D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
		(heapType == D3D12_HEAP_TYPE_DEFAULT) ? D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES : D3D12_HEAP_FLAG_NONE,
		&keep(CD3DX12_HEAP_PROPERTIES(heapType)),
		1) 
{

	m_bIsDynamicScaled = isDynamicScaled;
	m_uSize = count;
	m_uCapacity = count;
	m_iElemSizeInBytes = elemSizeInBytes;
	m_xResourceFlags = flags;
	m_xHeapType = heapType;
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
	if (m_uSize < 1) return;

	FResource::UploadData(pDevice, pCommandList, pData, state, false);


}

void StructuredBuffer::UploadRegion(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, const void* pData, UINT elementIndex, UINT numElements, D3D12_RESOURCE_STATES state) {
	UINT requiredCapacity = elementIndex + numElements;

	if (requiredCapacity > m_uCapacity)
	{
		if (m_bIsDynamicScaled)
		{
			UINT newCapacity = std::max(static_cast<UINT>(m_uCapacity * 1.5f), requiredCapacity);
			RecreateBuffer(pDevice, pCommandList, newCapacity, true);
		}
		else
		{
			SAFE_ASSERT(false, "StructuredBuffer overflow in UploadRegion. Enable dynamic scaling or check index.");
		}
	}

	m_uSize = std::max(m_uSize, requiredCapacity);
	if (m_uSize < 1) return;

	const UINT offsetInBytes = elementIndex * m_iElemSizeInBytes;
	const UINT sizeInBytes = numElements * m_iElemSizeInBytes;


	FResource::UploadDataRegion(pDevice, pCommandList, pData, offsetInBytes, sizeInBytes, state);
}


UINT StructuredBuffer::GetCapacity() const
{
	return m_uCapacity;
}

UINT StructuredBuffer::GetSize() const
{
	return m_uSize;
}

void StructuredBuffer::ReserveSize(ID3D12Device* pDevice, UINT newCapacity)
{
	if (newCapacity < 1) m_uCapacity = 1;

	if (m_uCapacity == newCapacity)
	{
		m_uSize = 0;
		return;
	}

	m_uSize = 0;

	FResource::CreateTextureBuffer(
		pDevice,
		1u,
		DXGI_FORMAT_UNKNOWN,
		newCapacity * m_iElemSizeInBytes,
		1,
		DXGI_SAMPLE_DESC({ 1, 0 }),
		D3D12_RESOURCE_DIMENSION_BUFFER,
		m_xResourceFlags,
		D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
		(m_xHeapType == D3D12_HEAP_TYPE_DEFAULT) ? D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES : D3D12_HEAP_FLAG_NONE,
		&keep(CD3DX12_HEAP_PROPERTIES(m_xHeapType)),
		1
	);

	m_uCapacity = newCapacity;
}

void StructuredBuffer::ReserveSize(ID3D12Device* pDevice, UINT sizeInBytes, UINT elemSize) {
	ReserveSize(pDevice, CalculateElementsCount(sizeInBytes, elemSize));
}


void StructuredBuffer::ShrinkToFit(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList) {
	if (m_uSize < m_uCapacity) RecreateBuffer(pDevice, pCommandList, m_uSize, true);
}

void StructuredBuffer::Clear(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, D3D12_RESOURCE_STATES state)
{
	if (m_uCapacity == 0) return;

	std::vector<uint8_t> zeros(m_uCapacity * m_iElemSizeInBytes, 0);
	m_uSize = m_uCapacity;

	FResource::UploadData(pDevice, pCommandList, zeros.data(), state, false);
}

UINT StructuredBuffer::CalculateElementsCount(UINT sizeInBytes, UINT elemSize) {
	return (UINT)std::ceil((double)sizeInBytes / (double)elemSize);
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
		m_xResourceFlags,
		D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
		(m_xHeapType == D3D12_HEAP_TYPE_DEFAULT) ? D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES : D3D12_HEAP_FLAG_NONE,
		&keep(CD3DX12_HEAP_PROPERTIES(m_xHeapType)),
		1
	);

	m_uCapacity = newCapacity;

	if (preserveData && !oldData.empty()) {
		FResource::UploadData(pDevice, pCommandList, oldData.data(), GetCurrentState(), false);
	}
}


}