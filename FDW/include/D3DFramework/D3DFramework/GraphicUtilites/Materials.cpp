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
		if (IsHaveTexture(TextureType::ROUGHNESS) && IsHaveTexture(TextureType::METALNESS)) {
			if (GetResourceTexture(TextureType::ROUGHNESS) == GetResourceTexture(TextureType::METALNESS)) {
				m_bIsRoughnessAndMetalnessInOneTexture = true;
				return;
			}
		}
		m_bIsRoughnessAndMetalnessInOneTexture = false;
	}

	bool Material::IsRoughnessAndMetalnessInOneTexture()
	{
		return m_bIsRoughnessAndMetalnessInOneTexture;
	}

}
