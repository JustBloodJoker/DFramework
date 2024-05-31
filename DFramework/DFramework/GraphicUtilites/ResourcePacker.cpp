#include "../pch.h"
#include "ResourcePacker.h"

namespace FDW
{




	
	ResourcePacker::ResourcePacker(UINT descriptorSize, UINT descriptorsCount, UINT NodeMask, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, ID3D12Device* pDevice)
		: descriptorCount(descriptorsCount), currentIndex(0)
	{
		InitBufferDescriptorHeap(descriptorSize, descriptorsCount, NodeMask, type, flags, pDevice);
	}

	std::unique_ptr<BufferDescriptorHeap>& ResourcePacker::GetResult() 
	{
		return descriptorHeap;
	}

	void ResourcePacker::InitBufferDescriptorHeap(UINT descriptorSize, UINT descriptorsCount, UINT NodeMask, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, ID3D12Device* pDevice)
	{
		CONSOLE_MESSAGE(std::string("RESOURCE PACKER INIT DESCRIPTOR HEAP"));
		descriptorHeap = std::make_unique<BufferDescriptorHeap>(descriptorSize, descriptorsCount > 0 ? descriptorsCount : 1U, NodeMask, type, flags, pDevice);
	}


	SRVPacker::SRVPacker(UINT descriptorSize, UINT descriptorsCount, UINT NodeMask, D3D12_DESCRIPTOR_HEAP_FLAGS flags, ID3D12Device* pDevice)
		: ResourcePacker(descriptorSize, descriptorsCount, NodeMask, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, flags, pDevice)
	{

	}

	void SRVPacker::AddResource(ID3D12Resource* resource, const D3D12_SRV_DIMENSION dimension, const size_t index, ID3D12Device* pDevice)
	{
		CONSOLE_MESSAGE(std::string("SRVPaker is adding resource"));


		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = resource->GetDesc().Format;
		srvDesc.ViewDimension = dimension;
		srvDesc.Texture2D.MostDetailedMip = 0; 
		srvDesc.Texture2D.MipLevels = resource->GetDesc().MipLevels;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		pDevice->CreateShaderResourceView(resource,
			&srvDesc, descriptorHeap->GetCPUDescriptorHandle(index < descriptorCount ? index : 0));
	}

	void SRVPacker::PushResource(ID3D12Resource* resource, const D3D12_SRV_DIMENSION dimension, ID3D12Device* pDevice)
	{
		AddResource(resource, dimension, currentIndex++, pDevice);
	}

	SamplerPacker::SamplerPacker(UINT descriptorSize, UINT descriptorsCount, UINT NodeMask,  D3D12_DESCRIPTOR_HEAP_FLAGS flags, ID3D12Device* pDevice)
		: ResourcePacker(descriptorSize, descriptorsCount, NodeMask, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, flags, pDevice)
	{

	}

	void SamplerPacker::AddResource(D3D12_SAMPLER_DESC desc, const size_t index, ID3D12Device* pDevice)
	{
		CONSOLE_MESSAGE(std::string("SAMPLERPaker is adding resource"));
		pDevice->CreateSampler(&desc, descriptorHeap->GetCPUDescriptorHandle(index < descriptorCount ? index : 0));
	}

	void SamplerPacker::PushResource(D3D12_SAMPLER_DESC desc, ID3D12Device* pDevice)
	{
		AddResource(desc, currentIndex++, pDevice);
	}

	void SamplerPacker::AddDefaultSampler(const size_t index, ID3D12Device* pDevice)
	{
		D3D12_SAMPLER_DESC desc = {};
		ZeroMemory(&desc, sizeof(desc));
		desc.Filter = D3D12_FILTER_ANISOTROPIC;
		desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		desc.MipLODBias = 0;
		desc.MaxAnisotropy = D3D12_DEFAULT_MAX_ANISOTROPY;
		desc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		desc.MinLOD = 0;
		desc.MaxLOD = D3D12_FLOAT32_MAX;

		this->AddResource(desc, 0, pDevice);
	}

	void SamplerPacker::PushDefaultSampler(ID3D12Device* pDevice)
	{
		AddDefaultSampler(currentIndex++, pDevice);
	}

	CBVPacker::CBVPacker(UINT descriptorSize, UINT descriptorsCount, UINT NodeMask, D3D12_DESCRIPTOR_HEAP_FLAGS flags, ID3D12Device* pDevice)
		: ResourcePacker(descriptorSize, descriptorsCount, NodeMask, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, flags, pDevice)
	{

	}

	void CBVPacker::AddResource(ID3D12Resource* resource, UINT sizeInBytes, const size_t index, ID3D12Device* pDevice)
	{
		CONSOLE_MESSAGE(std::string("CBVPaker is adding resource"));
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
		cbvDesc.BufferLocation = resource->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = sizeInBytes;
		pDevice->CreateConstantBufferView(&cbvDesc, descriptorHeap->GetCPUDescriptorHandle(index < descriptorCount ? index : 0));
	}

	void CBVPacker::PushResource(ID3D12Resource* resource, UINT sizeInBytes, ID3D12Device* pDevice)
	{
		AddResource(resource, sizeInBytes, currentIndex++, pDevice);
	}

	RTVPacker::RTVPacker(UINT descriptorSize, UINT descriptorsCount, UINT NodeMask, const D3D12_DESCRIPTOR_HEAP_FLAGS flags, ID3D12Device* pDevice)
		: ResourcePacker(descriptorSize, descriptorsCount, NodeMask, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, flags, pDevice)
	{
	}

	void RTVPacker::AddResource(ID3D12Resource* resource, const D3D12_RENDER_TARGET_VIEW_DESC rtvDesc, const size_t index, ID3D12Device* pDevice)
	{
		CONSOLE_MESSAGE(std::string("RTVPaker is adding resource"));
		pDevice->CreateRenderTargetView(resource, &rtvDesc, descriptorHeap->GetCPUDescriptorHandle(index < descriptorCount ? index : 0));

	}

	void RTVPacker::PushResource(ID3D12Resource* resource, const D3D12_RENDER_TARGET_VIEW_DESC rtvDesc, ID3D12Device* pDevice)
	{
		AddResource(resource, rtvDesc, currentIndex++, pDevice);
	}

	DSVPacker::DSVPacker(UINT descriptorSize, UINT descriptorsCount, UINT NodeMask, D3D12_DESCRIPTOR_HEAP_FLAGS flags, ID3D12Device* pDevice)
		: ResourcePacker(descriptorSize, descriptorsCount, NodeMask, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, flags, pDevice)
	{
	}

	void DSVPacker::AddResource(ID3D12Resource* resource, const D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc, const size_t index, ID3D12Device* pDevice)
	{
		CONSOLE_MESSAGE(std::string("DSVPaker is adding resource"));
		pDevice->CreateDepthStencilView(resource, &dsvDesc, descriptorHeap->GetCPUDescriptorHandle(index < descriptorCount ? index : 0));

	}

	void DSVPacker::PushResource(ID3D12Resource* resource, const D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc, ID3D12Device* pDevice)
	{
		AddResource(resource, dsvDesc, currentIndex++, pDevice);
	}

	UAVPacker::UAVPacker(UINT descriptorSize, UINT descriptorsCount, UINT NodeMask, D3D12_DESCRIPTOR_HEAP_FLAGS flags, ID3D12Device* pDevice)
		: ResourcePacker(descriptorSize, descriptorsCount, NodeMask, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, flags, pDevice)
	{
	}

	void UAVPacker::AddResource(ID3D12Resource* resource, ID3D12Resource* counterResource, size_t numElements, size_t firstElement, size_t stride, size_t offsetBytes, const size_t index, ID3D12Device* pDevice)
	{
		CONSOLE_MESSAGE(std::string("UAVPaker is adding resource"));

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc;
		ZeroMemory(&uavDesc, sizeof(uavDesc));
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = firstElement;
		uavDesc.Buffer.NumElements = numElements;
		uavDesc.Buffer.StructureByteStride = stride;
		uavDesc.Buffer.CounterOffsetInBytes = offsetBytes;
		uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

		pDevice->CreateUnorderedAccessView(resource, counterResource, &uavDesc, descriptorHeap->GetCPUDescriptorHandle(index));
	}

	void UAVPacker::PushResource(ID3D12Resource* resource, ID3D12Resource* counterResource, size_t numElements, size_t firstElement, size_t stride, size_t offsetBytes, ID3D12Device* pDevice)
	{
		AddResource(resource, counterResource, numElements, firstElement, stride, offsetBytes, currentIndex++, pDevice);
	}


}