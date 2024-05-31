#pragma once
#include "../pch.h"

#include "../GraphicUtilites/BufferMananger.h"

namespace FDW
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


	class Texture
	{

		static std::unordered_map<std::string, Texture*> textures;

	public:

		Texture(std::string path, ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList);
		Texture(ID3D12Device* pDevice, const UINT16 arraySize, const DXGI_FORMAT format,
			const UINT64 width, const UINT64 height, 
			DXGI_SAMPLE_DESC sampleDesc, const D3D12_RESOURCE_DIMENSION dimension, 
			const D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE,
			const D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
			const D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_NONE, 
			const D3D12_HEAP_PROPERTIES* heapProperties = &keep(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)),
			const UINT16 mipLevels = 1);
		
		~Texture();

		ID3D12Resource* GetResource() const;
		void ResourceBarrierChange(ID3D12GraphicsCommandList* pCommandList, const UINT numBariers, const D3D12_RESOURCE_STATES resourceStateAfter);
		
		void UploadData(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, const void* pData);

		static void ReleaseUploadBuffers();

	private:

		void CreateTextureBuffer(ID3D12Device* pDevice,
			const UINT16 arraySize, 
			const DXGI_FORMAT format, 
			const UINT64 width, 
			const UINT64 height, 
			DXGI_SAMPLE_DESC sampleDesc, 
			const D3D12_RESOURCE_DIMENSION dimension, 
			const D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE, 
			const D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN, 
			const D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_NONE, 
			const D3D12_HEAP_PROPERTIES* heapProperties = &keep(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)),
			const UINT16 mipLevels = 1);
		
		D3D12_RESOURCE_STATES currState;

		wrl::ComPtr<ID3D12Resource> resource;
		std::unique_ptr<UploadBuffer<char>> upBuffer;

	};

}