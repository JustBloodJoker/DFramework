#include "../pch.h"
#include "ResourcePacker.h"

namespace FD3DW
{




	
	ResourcePacker::ResourcePacker(UINT descriptorSize, UINT descriptorsCount, UINT NodeMask, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, ID3D12Device* pDevice)
		: m_uDescriptorCount(descriptorsCount), m_uCurrentIndex(0)
	{
		InitBufferDescriptorHeap(descriptorSize, descriptorsCount, NodeMask, type, flags, pDevice);
	}


	void ResourcePacker::ResizeHeap(UINT newDescriptorCount, ID3D12Device* pDevice) {

		auto newHeap = std::make_unique<BufferDescriptorHeap>( m_pDescriptorHeap->GetDescriptorSize(),newDescriptorCount, 0, m_pDescriptorHeap->GetType(),m_pDescriptorHeap->GetFlags(),pDevice);
		pDevice->CopyDescriptorsSimple( (UINT)m_uDescriptorCount, newHeap->GetCPUDescriptorHandle(0), m_pDescriptorHeap->GetCPUDescriptorHandle(0), m_pDescriptorHeap->GetType());
		m_pDescriptorHeap = std::move(newHeap);
		m_uDescriptorCount = newDescriptorCount;
	}

	std::unique_ptr<BufferDescriptorHeap>& ResourcePacker::GetResult() 
	{
		return m_pDescriptorHeap;
	}

	void ResourcePacker::InitBufferDescriptorHeap(UINT descriptorSize, UINT descriptorsCount, UINT NodeMask, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, ID3D12Device* pDevice)
	{
		if (!pDevice) return;
		m_uDescriptorCount = descriptorsCount;
		CONSOLE_MESSAGE(std::string("RESOURCE PACKER INIT DESCRIPTOR HEAP"));
		m_pDescriptorHeap = std::make_unique<BufferDescriptorHeap>(descriptorSize, descriptorsCount > 0 ? descriptorsCount : 1U, NodeMask, type, flags, pDevice);
	}


	std::unique_ptr<SRV_UAVPacker> SRV_UAVPacker::CreatePack(const UINT descriptorSize, const UINT descriptorsCount, const UINT NodeMask, const D3D12_DESCRIPTOR_HEAP_FLAGS flags, ID3D12Device* pDevice)
	{
		return std::make_unique<SRV_UAVPacker>(descriptorSize, descriptorsCount, NodeMask, flags, pDevice);
	}

	SRV_UAVPacker::SRV_UAVPacker(UINT descriptorSize, UINT descriptorsCount, UINT NodeMask, D3D12_DESCRIPTOR_HEAP_FLAGS flags, ID3D12Device* pDevice)
		: ResourcePacker(descriptorSize, descriptorsCount, NodeMask, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, flags, pDevice)
	{

	}

	void SRV_UAVPacker::AddResource(ID3D12Resource* resource, const D3D12_SRV_DIMENSION dimension, const size_t index, ID3D12Device* pDevice)
	{
		CONSOLE_MESSAGE(std::string("SRV_UAVPacker is adding resource"));

		const auto& desc = resource->GetDesc();

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = desc.Format;
		srvDesc.ViewDimension = dimension;

		switch (dimension)
		{
		case D3D12_SRV_DIMENSION_TEXTURE1D:
			srvDesc.Texture1D.MostDetailedMip = 0;
			srvDesc.Texture1D.MipLevels = desc.MipLevels;
			srvDesc.Texture1D.ResourceMinLODClamp = 0.0f;
			break;

		case D3D12_SRV_DIMENSION_TEXTURE1DARRAY:
			srvDesc.Texture1DArray.MostDetailedMip = 0;
			srvDesc.Texture1DArray.MipLevels = desc.MipLevels;
			srvDesc.Texture1DArray.FirstArraySlice = 0;
			srvDesc.Texture1DArray.ArraySize = desc.DepthOrArraySize;
			srvDesc.Texture1DArray.ResourceMinLODClamp = 0.0f;
			break;

		case D3D12_SRV_DIMENSION_TEXTURE2D:
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.MipLevels = desc.MipLevels;
			srvDesc.Texture2D.PlaneSlice = 0;
			srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
			break;

		case D3D12_SRV_DIMENSION_TEXTURE2DARRAY:
			srvDesc.Texture2DArray.MostDetailedMip = 0;
			srvDesc.Texture2DArray.MipLevels = desc.MipLevels;
			srvDesc.Texture2DArray.FirstArraySlice = 0;
			srvDesc.Texture2DArray.ArraySize = desc.DepthOrArraySize;
			srvDesc.Texture2DArray.PlaneSlice = 0;
			srvDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
			break;

		case D3D12_SRV_DIMENSION_TEXTURE2DMS:
			// No fields need to be set for Texture2DMS
			break;

		case D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY:
			srvDesc.Texture2DMSArray.FirstArraySlice = 0;
			srvDesc.Texture2DMSArray.ArraySize = desc.DepthOrArraySize;
			break;

		case D3D12_SRV_DIMENSION_TEXTURE3D:
			srvDesc.Texture3D.MostDetailedMip = 0;
			srvDesc.Texture3D.MipLevels = desc.MipLevels;
			srvDesc.Texture3D.ResourceMinLODClamp = 0.0f;
			break;

		case D3D12_SRV_DIMENSION_TEXTURECUBE:
			srvDesc.TextureCube.MostDetailedMip = 0;
			srvDesc.TextureCube.MipLevels = desc.MipLevels;
			srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
			break;

		case D3D12_SRV_DIMENSION_TEXTURECUBEARRAY:
			srvDesc.TextureCubeArray.MostDetailedMip = 0;
			srvDesc.TextureCubeArray.MipLevels = desc.MipLevels;
			srvDesc.TextureCubeArray.First2DArrayFace = 0;
			srvDesc.TextureCubeArray.NumCubes = desc.DepthOrArraySize / 6;
			srvDesc.TextureCubeArray.ResourceMinLODClamp = 0.0f;
			break;

		default:
			CONSOLE_ERROR_MESSAGE("SRV_UAVPacker: Unsupported SRV dimension");
			break;
		}

		pDevice->CreateShaderResourceView(resource,
			&srvDesc, m_pDescriptorHeap->GetCPUDescriptorHandle(index < m_uDescriptorCount ? (UINT)index : 0));
	}

	void SRV_UAVPacker::PushResource(ID3D12Resource* resource, const D3D12_SRV_DIMENSION dimension, ID3D12Device* pDevice)
	{
		AddResource(resource, dimension, m_uCurrentIndex++, pDevice);
	}

	void SRV_UAVPacker::AddNullResource(const size_t index, ID3D12Device* pDevice)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.PlaneSlice = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

		pDevice->CreateShaderResourceView(nullptr, &srvDesc, m_pDescriptorHeap->GetCPUDescriptorHandle(index < m_uDescriptorCount ? (UINT)index : 0));
	}

	void SRV_UAVPacker::AddResource(const UAVResourceDesc& desc, size_t index, ID3D12Device* pDevice)
	{
		CONSOLE_MESSAGE("SRV_UAVPacker is adding resource");

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = desc.Format;
		uavDesc.ViewDimension = desc.ViewDimension;

		switch (desc.ViewDimension)
		{
		case D3D12_UAV_DIMENSION_BUFFER:
			uavDesc.Buffer.FirstElement = desc.FirstElement;
			uavDesc.Buffer.NumElements = desc.NumElements;
			uavDesc.Buffer.StructureByteStride = desc.Stride;
			uavDesc.Buffer.CounterOffsetInBytes = desc.CounterOffsetBytes;
			uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
			break;

		case D3D12_UAV_DIMENSION_TEXTURE1D:
			uavDesc.Texture1D.MipSlice = desc.MipSlice;
			break;

		case D3D12_UAV_DIMENSION_TEXTURE1DARRAY:
			uavDesc.Texture1DArray.MipSlice = desc.MipSlice;
			uavDesc.Texture1DArray.FirstArraySlice = desc.FirstArraySlice;
			uavDesc.Texture1DArray.ArraySize = desc.ArraySize;
			break;

		case D3D12_UAV_DIMENSION_TEXTURE2D:
			uavDesc.Texture2D.MipSlice = desc.MipSlice;
			uavDesc.Texture2D.PlaneSlice = desc.PlaneSlice;
			break;

		case D3D12_UAV_DIMENSION_TEXTURE2DARRAY:
			uavDesc.Texture2DArray.MipSlice = desc.MipSlice;
			uavDesc.Texture2DArray.FirstArraySlice = desc.FirstArraySlice;
			uavDesc.Texture2DArray.ArraySize = desc.ArraySize;
			uavDesc.Texture2DArray.PlaneSlice = desc.PlaneSlice;
			break;

		case D3D12_UAV_DIMENSION_TEXTURE3D:
			uavDesc.Texture3D.MipSlice = desc.MipSlice;
			uavDesc.Texture3D.FirstWSlice = desc.FirstWSlice;
			uavDesc.Texture3D.WSize = desc.WSize;
			break;

		default:
			CONSOLE_MESSAGE("Unsupported UAV dimension!");
			return;
		}

		auto handle = m_pDescriptorHeap->GetCPUDescriptorHandle(index < m_uDescriptorCount ? (UINT)index : 0);
		pDevice->CreateUnorderedAccessView(desc.Resource, desc.CounterResource, &uavDesc, handle);

	}

	void SRV_UAVPacker::PushResource(const UAVResourceDesc& desc, ID3D12Device* pDevice)
	{
		AddResource(desc, m_uCurrentIndex++, pDevice);
	}

	std::unique_ptr<SamplerPacker> SamplerPacker::CreatePack(const UINT descriptorSize, const UINT descriptorsCount, const UINT NodeMask, const D3D12_DESCRIPTOR_HEAP_FLAGS flags, ID3D12Device* pDevice)
	{
		return std::make_unique<SamplerPacker>(descriptorSize, descriptorsCount, NodeMask, flags, pDevice);
	}

	SamplerPacker::SamplerPacker(UINT descriptorSize, UINT descriptorsCount, UINT NodeMask,  D3D12_DESCRIPTOR_HEAP_FLAGS flags, ID3D12Device* pDevice)
		: ResourcePacker(descriptorSize, descriptorsCount, NodeMask, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, flags, pDevice)
	{

	}

	void SamplerPacker::AddResource(D3D12_SAMPLER_DESC desc, const size_t index, ID3D12Device* pDevice)
	{
		CONSOLE_MESSAGE(std::string("SAMPLERPaker is adding resource"));
		pDevice->CreateSampler(&desc, m_pDescriptorHeap->GetCPUDescriptorHandle(index < m_uDescriptorCount ? (UINT)index : 0));
	}

	void SamplerPacker::PushResource(D3D12_SAMPLER_DESC desc, ID3D12Device* pDevice)
	{
		AddResource(desc, m_uCurrentIndex++, pDevice);
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
		AddDefaultSampler(m_uCurrentIndex++, pDevice);
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
		pDevice->CreateConstantBufferView(&cbvDesc, m_pDescriptorHeap->GetCPUDescriptorHandle(index < m_uDescriptorCount ? (UINT)index : 0));
	}

	void CBVPacker::PushResource(ID3D12Resource* resource, UINT sizeInBytes, ID3D12Device* pDevice)
	{
		AddResource(resource, sizeInBytes, m_uCurrentIndex++, pDevice);
	}

	RTVPacker::RTVPacker(UINT descriptorSize, UINT descriptorsCount, UINT NodeMask, const D3D12_DESCRIPTOR_HEAP_FLAGS flags, ID3D12Device* pDevice)
		: ResourcePacker(descriptorSize, descriptorsCount, NodeMask, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, flags, pDevice)
	{
	}

	void RTVPacker::AddResource(ID3D12Resource* resource, const D3D12_RENDER_TARGET_VIEW_DESC rtvDesc, const size_t index, ID3D12Device* pDevice)
	{
		CONSOLE_MESSAGE(std::string("RTVPaker is adding resource"));
		pDevice->CreateRenderTargetView(resource, &rtvDesc, m_pDescriptorHeap->GetCPUDescriptorHandle(index < m_uDescriptorCount ? (UINT)index : 0));

	}

	void RTVPacker::PushResource(ID3D12Resource* resource, const D3D12_RENDER_TARGET_VIEW_DESC rtvDesc, ID3D12Device* pDevice)
	{
		AddResource(resource, rtvDesc, m_uCurrentIndex++, pDevice);
	}

	DSVPacker::DSVPacker(UINT descriptorSize, UINT descriptorsCount, UINT NodeMask, D3D12_DESCRIPTOR_HEAP_FLAGS flags, ID3D12Device* pDevice)
		: ResourcePacker(descriptorSize, descriptorsCount, NodeMask, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, flags, pDevice)
	{
	}

	void DSVPacker::AddResource(ID3D12Resource* resource, const D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc, const size_t index, ID3D12Device* pDevice)
	{
		
		CONSOLE_MESSAGE(std::string("DSVPaker is adding resource"));
		pDevice->CreateDepthStencilView(resource, &dsvDesc, m_pDescriptorHeap->GetCPUDescriptorHandle(index < m_uDescriptorCount ? (UINT)index : 0));

	}

	void DSVPacker::PushResource(ID3D12Resource* resource, const D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc, ID3D12Device* pDevice)
	{
		AddResource(resource, dsvDesc, m_uCurrentIndex++, pDevice);
	}



}