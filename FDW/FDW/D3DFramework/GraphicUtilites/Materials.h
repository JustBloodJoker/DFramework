#pragma once

#include "../pch.h"
#include "FResource.h"

namespace FD3DW
{
	struct MaterialFrameWork
	{
		MaterialFrameWork()
		{
			ZeroMemory(this, sizeof(MaterialFrameWork));
		}
		dx::XMFLOAT4 Diffuse;
		dx::XMFLOAT4 Ambient;
		dx::XMFLOAT4 Emissive;
		dx::XMFLOAT4 Specular;
	};

	class Material
	{

	public:

		Material(MaterialFrameWork&& materialDesc);
		Material(const MaterialFrameWork& materialDesc);
		Material();
		~Material()=default;

		ID3D12Resource* GetResourceTexture(TextureType type) const;
		MaterialFrameWork GetMaterialDesc() const;

		template<typename MaterialFW>
		void SetMaterialDesc(MaterialFW&& matdesc)
		{
			m_xMaterial = std::forward<decltype(matdesc)>(matdesc);
		}

		void SetTexture(std::string& texturePath, TextureType type, ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList);

		bool IsHaveTexture(TextureType type) const;

	private:

		MaterialFrameWork m_xMaterial;
		std::unordered_map<TextureType, std::shared_ptr<FResource>> m_mTextureMap;


	};


}
