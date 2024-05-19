#pragma once
#include "../pch.h"

#include "BufferDescriptorHeap.h"


namespace FDW
{ 


	class ResourcePacker
	{
		ResourcePacker();
	public:

		ResourcePacker(UINT descriptorSize, UINT descriptorsCount, UINT NodeMask, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, ID3D12Device* pDevice);
		virtual ~ResourcePacker() = default;

		std::unique_ptr<BufferDescriptorHeap>& GetResult();

	protected:

		std::unique_ptr <BufferDescriptorHeap> descriptorHeap;

		const size_t descriptorCount;
	private:

		void InitBufferDescriptorHeap(UINT descriptorSize, UINT descriptorsCount, UINT NodeMask, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, ID3D12Device* pDevice);

	};

	class SRVPacker
		: public ResourcePacker
	{

	public:

		SRVPacker(UINT descriptorSize, UINT descriptorsCount, UINT NodeMask, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, ID3D12Device* pDevice);

		void AddResource(ID3D12Resource* resource, const D3D12_SRV_DIMENSION dimension, const size_t index, ID3D12Device* pDevice);

	};

	class SamplerPacker
		: public ResourcePacker
	{
	public:

		SamplerPacker(UINT descriptorSize, UINT descriptorsCount, UINT NodeMask, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, ID3D12Device* pDevice);

		void AddResource(D3D12_SAMPLER_DESC desc, const size_t index, ID3D12Device* pDevice);
		
		void AddDefaultSampler(const size_t index, ID3D12Device* pDevice);
	
	};

	class CBVPacker
		: public ResourcePacker
	{
	public:
		CBVPacker(UINT descriptorSize, UINT descriptorsCount, UINT NodeMask, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, ID3D12Device* pDevice);

		void AddResource(ID3D12Resource* resource, UINT sizeInBytes, const size_t index, ID3D12Device* pDevice);


	};

	class RTVPacker
		: public ResourcePacker
	{
	public:
		RTVPacker(UINT descriptorSize, UINT descriptorsCount, UINT NodeMask, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, ID3D12Device* pDevice);
		void AddResource(ID3D12Resource* resource, const D3D12_RENDER_TARGET_VIEW_DESC rtvDesc, const size_t index, ID3D12Device* pDevice);


	};

	class DSVPacker
		: public ResourcePacker
	{
	public:
		DSVPacker(UINT descriptorSize, UINT descriptorsCount, UINT NodeMask, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, ID3D12Device* pDevice);
		void AddResource(ID3D12Resource* resource, const D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc, const size_t index, ID3D12Device* pDevice);


	};

}
