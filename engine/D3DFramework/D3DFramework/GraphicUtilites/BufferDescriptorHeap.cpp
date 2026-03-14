#include "../pch.h"
#include "BufferDescriptorHeap.h"


namespace FD3DW
{


	BufferDescriptorHeap::BufferDescriptorHeap(UINT descriptorSize, UINT descriptorsCount, UINT NodeMask, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, ID3D12Device* pDevice) : m_uDescriptorSize(descriptorSize), m_uDescriptorsCount(descriptorsCount)
	{
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
		heapDesc.NumDescriptors = descriptorsCount;
		heapDesc.Type = type;
		heapDesc.Flags = flags;
		heapDesc.NodeMask = NodeMask;

		m_xType = type;
		m_xFlags = flags;

		HRESULT_ASSERT(pDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(m_pDescriptorHeap.ReleaseAndGetAddressOf())), "Create descriptor heap error");
	}

	ID3D12DescriptorHeap* FD3DW::BufferDescriptorHeap::GetDescriptorPtr() const noexcept
	{
		return m_pDescriptorHeap.Get();
	}

	D3D12_GPU_DESCRIPTOR_HANDLE BufferDescriptorHeap::GetGPUDescriptorHandle(UINT offset) const noexcept
	{
		CD3DX12_GPU_DESCRIPTOR_HANDLE pdhHandle(m_pDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		pdhHandle.Offset(offset < m_uDescriptorsCount ? offset :  0, m_uDescriptorSize);
		return pdhHandle;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE BufferDescriptorHeap::GetCPUDescriptorHandle(UINT offset) const noexcept
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE pdhHandle(m_pDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
		pdhHandle.Offset(offset < m_uDescriptorsCount ? offset : 0, m_uDescriptorSize);
		return pdhHandle;
	}

	UINT BufferDescriptorHeap::GetDescriptorSize()
	{
		return m_uDescriptorSize;
	}

	D3D12_DESCRIPTOR_HEAP_TYPE BufferDescriptorHeap::GetType()
	{
		return m_xType;
	}

	D3D12_DESCRIPTOR_HEAP_FLAGS BufferDescriptorHeap::GetFlags()
	{
		return m_xFlags;
	}


}