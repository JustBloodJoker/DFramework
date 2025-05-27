#include "../pch.h"
#include "Materials.h"

namespace FD3DW
{

	Material::Material(MaterialFrameWork&& materialDesc) : m_xMaterial(std::move(materialDesc))
	{
	}

	Material::Material(const MaterialFrameWork& materialDesc) : m_xMaterial(materialDesc)
	{
	}

	Material::Material() : m_xMaterial(MaterialFrameWork())
	{
	}

	ID3D12Resource* Material::GetResourceTexture(TextureType type) const
	{
		return m_mTextureMap.at(type)->GetResource();
	}

	MaterialFrameWork Material::GetMaterialDesc() const
	{
		return m_xMaterial;
	}

	void Material::SetTexture(std::string& texturePath, TextureType type, ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList)
	{
		m_mTextureMap.emplace(type,FResource::CreateTextureFromPath(texturePath,pDevice, pCommandList));
	}

	bool Material::IsHaveTexture(TextureType type) const
	{
		return m_mTextureMap.find(type) != m_mTextureMap.end();
	}

}
