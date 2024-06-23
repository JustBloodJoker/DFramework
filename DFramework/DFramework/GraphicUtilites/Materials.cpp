#include "../pch.h"
#include "Materials.h"

namespace FDW
{

	Material::Material(MaterialFrameWork&& materialDesc) : material(std::move(materialDesc))
	{
	}

	Material::Material(const MaterialFrameWork& materialDesc) : material(materialDesc)
	{
	}

	Material::Material() : material(MaterialFrameWork())
	{
	}

	Material::~Material()
	{
		for(auto &el : textureMap)
		{
			el.second.reset();
		}
	}

	ID3D12Resource* Material::GetResourceTexture(TextureType type) const
	{
		return textureMap.at(type)->GetResource();
	}

	MaterialFrameWork Material::GetMaterialDesc() const
	{
		return material;
	}

	void Material::SetTexture(std::string& texturePath, TextureType type, ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList)
	{
		textureMap.emplace(type,Texture::CreateTextureFromPath(texturePath,pDevice, pCommandList));
	}

	bool Material::IsHaveTexture(TextureType type) const
	{
		return textureMap.find(type) != textureMap.end();
	}

}
