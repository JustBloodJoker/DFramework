#pragma once

#include "../pch.h"
#include "BufferManager.h"

namespace FD3DW {

	struct TextureDataOutput
	{
		wrl::ComPtr<ID3D12Resource> TextureResource;
		std::unique_ptr<UploadBuffer<char>> ResultUploadBuffer;
	};

	class DDSTextureLoaderDX12
	{
	public:

		static HRESULT Load(const std::string& path,ID3D12Device* device,ID3D12GraphicsCommandList* commandList,TextureDataOutput& outTexture);
	};


}
