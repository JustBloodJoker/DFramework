#pragma once
#include "../pch.h"

#include "../GraphicUtilites/BufferMananger.h"
#include "../GraphicUtilites/PostProcessing.h"



namespace FD3DW
{

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
			EMISSIVE
		};
	}

	class FResource : virtual public PostProcessing, virtual public std::enable_shared_from_this<FResource>
	{

		static std::unordered_map<std::string, std::weak_ptr<FResource>> textures;
		
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
		
		void UploadData(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, const void* pData, bool checkCalculation = false);

		bool DeleteUploadBuffer();

		static void ReleaseUploadBuffers();//CLEAR ALL UPLOAD BUFFERS

	private:

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
			const UINT16 mipLevels = 1);
		
		D3D12_RESOURCE_STATES currState;

		wrl::ComPtr<ID3D12Resource> pResource;
		std::unique_ptr<UploadBuffer<char>> upBuffer;

		std::string Path;
	};

}

namespace TextureTypeSpace = FD3DW::TEXTURETYPE;
using TextureType = FD3DW::TEXTURETYPE::TEXTURE_TYPE;
