#pragma once

#include "../pch.h"
#include "Texture.h"

namespace FDW
{
	struct MaterialFrameWork
	{
		MaterialFrameWork()
		{
			ZeroMemory(this, sizeof(MaterialFrameWork));
		}
		dx::XMFLOAT4 diffuse;
		dx::XMFLOAT4 ambient;
		dx::XMFLOAT4 emissive;
		dx::XMFLOAT4 specular;
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
			material = std::forward<decltype(matdesc)>(matdesc);
		}

		void SetTexture(std::string& texturePath, TextureType type, ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList);

		bool IsHaveTexture(TextureType type) const;

	private:

		MaterialFrameWork material;
		std::unordered_map<TextureType, std::shared_ptr<Texture>> textureMap;


	};


}
