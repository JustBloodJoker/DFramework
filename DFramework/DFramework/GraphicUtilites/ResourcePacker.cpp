#include "../pch.h"
#include "ResourcePacker.h"

namespace FDW
{




	
	ResourcePacker::ResourcePacker(UINT descriptorSize, UINT descriptorsCount, UINT NodeMask, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, ID3D12Device* pDevice)
		: descriptorCount(descriptorsCount)
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


	SRVPacker::SRVPacker(UINT descriptorSize, UINT descriptorsCount, UINT NodeMask, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, ID3D12Device* pDevice)
		: ResourcePacker(descriptorSize, descriptorsCount, NodeMask, type, flags, pDevice)
	{

	}

	void SRVPacker::AddResource(ID3D12Resource* resource, const D3D12_SRV_DIMENSION dimension, const size_t index, ID3D12Device* pDevice)
	{
		CONSOLE_MESSAGE(std::string("SRVPACKER ADDING RESOURCE"));


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

	SamplerPacker::SamplerPacker(UINT descriptorSize, UINT descriptorsCount, UINT NodeMask, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, ID3D12Device* pDevice)
		: ResourcePacker(descriptorSize, descriptorsCount, NodeMask, type, flags, pDevice)
	{

	}

	void SamplerPacker::AddResource(D3D12_SAMPLER_DESC desc, const size_t index, ID3D12Device* pDevice)
	{
		CONSOLE_MESSAGE(std::string("SAMPLERPACKER ADDING RESOURCE"));
		pDevice->CreateSampler(&desc, descriptorHeap->GetCPUDescriptorHandle(index < descriptorCount ? index : 0));
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

	CBVPacker::CBVPacker(UINT descriptorSize, UINT descriptorsCount, UINT NodeMask, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, ID3D12Device* pDevice)
		: ResourcePacker(descriptorSize, descriptorsCount, NodeMask, type, flags, pDevice)
	{

	}

	void CBVPacker::AddResource(ID3D12Resource* resource, UINT sizeInBytes, const size_t index, ID3D12Device* pDevice)
	{
		CONSOLE_MESSAGE(std::string("CBVPACKER ADDING RESOURCE"));
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
		cbvDesc.BufferLocation = resource->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = sizeInBytes;
		pDevice->CreateConstantBufferView(&cbvDesc, descriptorHeap->GetCPUDescriptorHandle(index < descriptorCount ? index : 0));
	}

	RTVPacker::RTVPacker(UINT descriptorSize, UINT descriptorsCount, UINT NodeMask, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, ID3D12Device* pDevice)
		: ResourcePacker(descriptorSize, descriptorsCount, NodeMask, type, flags, pDevice)
	{
	}

	void RTVPacker::AddResource(ID3D12Resource* resource, const D3D12_RENDER_TARGET_VIEW_DESC rtvDesc, const size_t index, ID3D12Device* pDevice)
	{
		CONSOLE_MESSAGE(std::string("RTVPACKER ADDING RESOURCE"));
		pDevice->CreateRenderTargetView(resource, &rtvDesc, descriptorHeap->GetCPUDescriptorHandle(index < descriptorCount ? index : 0));

	}

	DSVPacker::DSVPacker(UINT descriptorSize, UINT descriptorsCount, UINT NodeMask, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, ID3D12Device* pDevice)
		: ResourcePacker(descriptorSize, descriptorsCount, NodeMask, type, flags, pDevice)
	{
	}

	void DSVPacker::AddResource(ID3D12Resource* resource, const D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc, const size_t index, ID3D12Device* pDevice)
	{
		CONSOLE_MESSAGE(std::string("DSVPACKER ADDING RESOURCE"));
		pDevice->CreateDepthStencilView(resource, &dsvDesc, descriptorHeap->GetCPUDescriptorHandle(index < descriptorCount ? index : 0));

	}

}