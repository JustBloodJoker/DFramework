#pragma once
#include "../pch.h"

#include "BufferDescriptorHeap.h"


namespace FDW
{ 


	class ResourcePacker
	{
		ResourcePacker();
	public:

		ResourcePacker(const UINT descriptorSize, const  UINT descriptorsCount, const  UINT NodeMask, const D3D12_DESCRIPTOR_HEAP_TYPE type, const D3D12_DESCRIPTOR_HEAP_FLAGS flags, ID3D12Device* pDevice);
		virtual ~ResourcePacker() = default;

		std::unique_ptr<BufferDescriptorHeap>& GetResult();

	protected:

		std::unique_ptr <BufferDescriptorHeap> descriptorHeap;

		const size_t descriptorCount;
		size_t currentIndex;

	private:

		void InitBufferDescriptorHeap(const UINT descriptorSize, const  UINT descriptorsCount, const UINT NodeMask, const D3D12_DESCRIPTOR_HEAP_TYPE type, const D3D12_DESCRIPTOR_HEAP_FLAGS flags, ID3D12Device* pDevice);

	};

	class SRVPacker
		: public ResourcePacker
	{

	public:

		SRVPacker(const UINT descriptorSize, const  UINT descriptorsCount, const UINT NodeMask, const D3D12_DESCRIPTOR_HEAP_FLAGS flags, ID3D12Device* pDevice);
		virtual ~SRVPacker() = default;

		void AddResource(ID3D12Resource* resource, const D3D12_SRV_DIMENSION dimension, const size_t index, ID3D12Device* pDevice);
		void PushResource(ID3D12Resource* resource, const D3D12_SRV_DIMENSION dimension, ID3D12Device* pDevice);
	};

	class SamplerPacker
		: public ResourcePacker
	{
	public:

		SamplerPacker(const UINT descriptorSize, const UINT descriptorsCount, const UINT NodeMask, const D3D12_DESCRIPTOR_HEAP_FLAGS flags, ID3D12Device* pDevice);
		virtual ~SamplerPacker() = default;

		void AddResource(const D3D12_SAMPLER_DESC desc, const size_t index, ID3D12Device* pDevice);
		void PushResource(const D3D12_SAMPLER_DESC desc, ID3D12Device* pDevice);
		
		void AddDefaultSampler(const size_t index, ID3D12Device* pDevice);
		void PushDefaultSampler(ID3D12Device* pDevice);
	
	};

	class CBVPacker
		: public ResourcePacker
	{
	public:
		CBVPacker(const UINT descriptorSize, const UINT descriptorsCount, const UINT NodeMask, const D3D12_DESCRIPTOR_HEAP_FLAGS flags, ID3D12Device* pDevice);
		virtual ~CBVPacker() = default;

		void AddResource(ID3D12Resource* resource, const UINT sizeInBytes, const size_t index, ID3D12Device* pDevice);
		void PushResource(ID3D12Resource* resource, const UINT sizeInBytes, ID3D12Device* pDevice);


	};

	class RTVPacker
		: public ResourcePacker
	{
	public:
		RTVPacker(UINT descriptorSize, UINT descriptorsCount, UINT NodeMask, const D3D12_DESCRIPTOR_HEAP_FLAGS flags, ID3D12Device* pDevice);
		virtual ~RTVPacker() = default;

		void AddResource(ID3D12Resource* resource, const D3D12_RENDER_TARGET_VIEW_DESC rtvDesc, const size_t index, ID3D12Device* pDevice);
		void PushResource(ID3D12Resource* resource, const D3D12_RENDER_TARGET_VIEW_DESC rtvDesc, ID3D12Device* pDevice);
	};

	class DSVPacker
		: public ResourcePacker
	{
	public:
		DSVPacker(UINT descriptorSize, UINT descriptorsCount, UINT NodeMask, const D3D12_DESCRIPTOR_HEAP_FLAGS flags, ID3D12Device* pDevice);
		virtual ~DSVPacker() = default;

		void AddResource(ID3D12Resource* resource, const D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc, const size_t index, ID3D12Device* pDevice);
		void PushResource(ID3D12Resource* resource, const D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc, ID3D12Device* pDevice);
	};

	class UAVPacker
		: public ResourcePacker
	{
	public:
		UAVPacker(UINT descriptorSize, UINT descriptorsCount, UINT NodeMask, const D3D12_DESCRIPTOR_HEAP_FLAGS flags, ID3D12Device* pDevice);
		virtual ~UAVPacker() = default;

		void AddResource(ID3D12Resource* resource, ID3D12Resource* counterResource, size_t numElements, size_t firstElements, size_t stride, size_t offsetBytes, const size_t index, ID3D12Device* pDevice);
		void PushResource(ID3D12Resource* resource, ID3D12Resource* counterResource, size_t numElements, size_t firstElements, size_t stride, size_t offsetBytes, ID3D12Device* pDevice);
	};

}
