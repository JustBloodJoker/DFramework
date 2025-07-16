#pragma once
#include "../pch.h"

#include "../GraphicUtilites/BufferManager.h"
#include "../GraphicUtilites/PostProcessing.h"



namespace FD3DW
{

	class PipelineObject;

	namespace TEXTURETYPE
	{
		enum TEXTURE_TYPE
		{
			BASE,
			NORMAL,
			ROUGHNESS,
			METALNESS,
			HEIGHT,
			SPECULAR,
			OPACITY,
			BUMP,
			EMISSIVE,

			SIZE
		};
	}

	class FResource : virtual public PostProcessing, virtual public std::enable_shared_from_this<FResource>
	{

		static std::unordered_map<std::string, std::weak_ptr<FResource>> s_vTextures;
		
	public:
		static std::unique_ptr<FResource> CreateSimpleStructuredBuffer(ID3D12Device* pDevice, const UINT width);
		static std::unique_ptr<FResource> CreateAnonimTexture(ID3D12Device* pDevice, const UINT16 arraySize, const DXGI_FORMAT format,const UINT width, const UINT height,DXGI_SAMPLE_DESC sampleDesc, const D3D12_RESOURCE_DIMENSION dimension,const D3D12_RESOURCE_FLAGS resourceFlags,const D3D12_TEXTURE_LAYOUT layout,const D3D12_HEAP_FLAGS heapFlags,const D3D12_HEAP_PROPERTIES* heapProperties,const UINT16 mipLevels);

	public:
		static PipelineObject* GetMipGenerationPSO(ID3D12Device* device);

	public:
		static std::shared_ptr<FResource> CreateTextureFromPath(std::string path, ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList);
		FResource(std::string path, ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList);//NOT RECOMENDED TO USE

		std::shared_ptr<FResource> GetSharedFromThis();

		FResource(ID3D12Device* pDevice, const UINT16 arraySize, const DXGI_FORMAT format,
			const UINT width, const UINT height,
			DXGI_SAMPLE_DESC sampleDesc, const D3D12_RESOURCE_DIMENSION dimension, 
			const D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE,
			const D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
			const D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_NONE, 
			const D3D12_HEAP_PROPERTIES* heapProperties = &keep(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)),
			const UINT16 mipLevels = 1);
		
		~FResource();

		ID3D12Resource* GetResource() const;
		void ResourceBarrierChange(ID3D12GraphicsCommandList* pCommandList, const UINT numBariers, const D3D12_RESOURCE_STATES resourceStateAfter);
		
		void UploadData(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, const void* pData, D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,bool bCopyAllMips=false);

		void GenerateMips(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList);

		bool DeleteUploadBuffer();

		static void ReleaseUploadBuffers();//CLEAR ALL UPLOAD BUFFERS

	private:
		UINT CalculateMipCount(UINT width, UINT height);
		bool IsSupportMipMapping(D3D12_RESOURCE_DESC desc);

		void CreateTextureBuffer(ID3D12Device* pDevice,
			const UINT16 arraySize, 
			const DXGI_FORMAT format, 
			const UINT width,
			const UINT height,
			DXGI_SAMPLE_DESC sampleDesc, 
			const D3D12_RESOURCE_DIMENSION dimension, 
			const D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE, 
			const D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN, 
			const D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_NONE, 
			const D3D12_HEAP_PROPERTIES* heapProperties = &keep(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)),
			const UINT16 mipLevels = 1, bool WillGeneratedMips = true);
		
		D3D12_RESOURCE_STATES m_xCurrState;

		wrl::ComPtr<ID3D12Resource> m_pResource;
		std::unique_ptr<UploadBuffer<char>> m_pUploadBuffer;

		std::string m_sPath;
	};

	namespace TextureTypeSpace = FD3DW::TEXTURETYPE;
	using TextureType = FD3DW::TEXTURETYPE::TEXTURE_TYPE;

}

