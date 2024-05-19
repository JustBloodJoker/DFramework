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
			el.second.release();
		}
	}

	ID3D12Resource* Material::GetResourceTexture(TEXTURE_TYPE type) const
	{
		return textureMap.at(type)->GetResource();
	}

	MaterialFrameWork Material::GetMaterialDesc() const
	{
		return material;
	}

	void Material::SetMaterialDesc(MaterialFrameWork&& materialDesc)
	{
		material = std::move(materialDesc);
	}
	void Material::SetMaterialDesc(const MaterialFrameWork& materialDesc)
	{
		material = materialDesc;
	}

	void Material::SetTexture(std::string& texturePath, TEXTURE_TYPE type, ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList)
	{
		
		textureMap[type] = std::make_unique<Texture>(texturePath,pDevice, pCommandList);
	}

	bool Material::IsHaveTexture(TEXTURE_TYPE type) const
	{
		return textureMap.find(type) != textureMap.end();
	}

}
