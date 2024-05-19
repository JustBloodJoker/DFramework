#include "../pch.h"
#include "BufferDescriptorHeap.h"


namespace FDW
{


	BufferDescriptorHeap::BufferDescriptorHeap(UINT descriptorSize, UINT descriptorsCount, UINT NodeMask, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, ID3D12Device* pDevice) : descriptorSize(descriptorSize), descriptorsCount(descriptorsCount)
	{
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
		heapDesc.NumDescriptors = descriptorsCount;
		heapDesc.Type = type;
		heapDesc.Flags = flags;
		heapDesc.NodeMask = NodeMask;

		HRESULT_ASSERT(pDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(pDescriptorHeap.GetAddressOf())), "Create descriptor heap error");
	}

	BufferDescriptorHeap::~BufferDescriptorHeap()
	{
	}

	ID3D12DescriptorHeap* FDW::BufferDescriptorHeap::GetDescriptorPtr() const noexcept
	{
		return pDescriptorHeap.Get();
	}

	D3D12_GPU_DESCRIPTOR_HANDLE BufferDescriptorHeap::GetGPUDescriptorHandle(UINT offset) const noexcept
	{
		CD3DX12_GPU_DESCRIPTOR_HANDLE pdhHandle(pDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		pdhHandle.Offset(offset < descriptorsCount ? offset :  0, descriptorSize);
		return pdhHandle;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE BufferDescriptorHeap::GetCPUDescriptorHandle(UINT offset) const noexcept
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE pdhHandle(pDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
		pdhHandle.Offset(offset < descriptorsCount ? offset : 0, descriptorSize);
		return pdhHandle;
	}


}