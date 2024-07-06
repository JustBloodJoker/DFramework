#pragma once
#include "../pch.h"


namespace FDW
{


	class BufferDescriptorHeap
	{

		BufferDescriptorHeap() = default;

	public:

		BufferDescriptorHeap(UINT descriptorSize, UINT descriptorsCount, UINT NodeMask, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, ID3D12Device* pDevice);
		
		~BufferDescriptorHeap()=default;

		ID3D12DescriptorHeap* GetDescriptorPtr() const noexcept;
		D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(UINT offset) const noexcept;
		D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(UINT offset) const noexcept;


	private:

		wrl::ComPtr<ID3D12DescriptorHeap> pDescriptorHeap;
		UINT descriptorsCount;
		UINT descriptorSize;

	};


}