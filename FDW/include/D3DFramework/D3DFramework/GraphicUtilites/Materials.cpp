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

	std::shared_ptr<FResource> Material::GetTexture(TextureType type) const
	{
		return m_mTextureMap.contains(type) ? m_mTextureMap.at(type) : nullptr;
	}

	ID3D12Resource* Material::GetResourceTexture(TextureType type) const
	{
		return m_mTextureMap.contains(type) ? m_mTextureMap.at(type)->GetResource() : nullptr;
	}

	MaterialFrameWork Material::GetMaterialDesc() const
	{
		return m_xMaterial;
	}

	void Material::SetTexture(const std::string& texturePath, TextureType type, ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList)
	{
		m_mTextureMap.insert_or_assign(type, FResource::CreateTextureFromPath(texturePath, pDevice, pCommandList));
		UpdateRoughnessMetalnessTexturesInfo();
	}

	void Material::DeleteTexture(TextureType type)
	{
		m_mTextureMap.erase(type);
	}

	bool Material::IsHaveTexture(TextureType type) const
	{
		return m_mTextureMap.find(type) != m_mTextureMap.end();
	}

	void Material::UpdateRoughnessMetalnessTexturesInfo()
	{
		auto hasRoughness = IsHaveTexture(TextureType::ROUGHNESS);
		auto hasMetalness = IsHaveTexture(TextureType::METALNESS);
		auto hasAO = IsHaveTexture(TextureType::AMBIENT);

		auto textureCount = int(hasRoughness) + int(hasMetalness) + int(hasAO);

		if (textureCount < 2) {
			m_bIsORMTextureType = false;
			return;
		}

		auto roughTex = hasRoughness ? GetResourceTexture(TextureType::ROUGHNESS) : nullptr;
		auto metalTex = hasMetalness ? GetResourceTexture(TextureType::METALNESS) : nullptr;
		auto aoTex = hasAO ? GetResourceTexture(TextureType::AMBIENT) : nullptr;

		auto anyTwoMatch = (roughTex && metalTex && roughTex == metalTex) ||
						   (roughTex && aoTex && roughTex == aoTex) ||
					       (metalTex && aoTex && metalTex == aoTex);

		m_bIsORMTextureType = anyTwoMatch;
	}

	bool Material::IsORMTextureType()
	{
		return m_bIsORMTextureType;
	}

}
