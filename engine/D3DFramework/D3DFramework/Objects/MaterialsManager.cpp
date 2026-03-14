#include "../pch.h"
#include "MaterialsManager.h"


namespace FD3DW
{

	void MaterialsManager::AddMaterial()
	{
		m_vMaterials.push_back(std::make_unique<Material>());
	}

	void MaterialsManager::SetTexture(std::string& texturePath, TextureType type, ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, const std::size_t index)
	{
		if (index < m_vMaterials.size() && index >= 0)
		{
			CONSOLE_MESSAGE(std::string("SET TEXTURE TO THE MATERIAL WITH INDEX " + std::to_string(index)).c_str());
			SetTexture(m_vMaterials[index], texturePath, type, pDevice, pCommandList);
		}
		else if(index == -1)
		{
			CONSOLE_MESSAGE(std::string("SET TEXTURE TO THE MATERIAL WITH INDEX " + std::to_string(m_vMaterials.size() - 1)).c_str());
			SetTexture(*m_vMaterials.rbegin(), texturePath, type, pDevice, pCommandList);
		}
		else
		{
			CONSOLE_MESSAGE(std::string("SET TEXTURE ERROR. CAN'T FIND MATERIAL WITH INDEX " + std::to_string(index)).c_str());
		}


	}

	void MaterialsManager::SetMaterialDesc(const MaterialFrameWork& materialDesc, const std::size_t index)
	{
		if (index < m_vMaterials.size() && index >= 0)
		{
			CONSOLE_MESSAGE(std::string("SET MATERIALDESC TO THE MATERIAL WITH INDEX " + std::to_string(index)).c_str());
			SetMaterialDesc(m_vMaterials[index], materialDesc);
		}
		else if (index == -1)
		{
			CONSOLE_MESSAGE(std::string("SET MATERIALDESC TO THE MATERIAL WITH INDEX " + std::to_string(m_vMaterials.size() - 1)).c_str());
			SetMaterialDesc(*m_vMaterials.rbegin(), materialDesc);
		}
		else
		{
			CONSOLE_MESSAGE(std::string("SET MATERIALDESC ERROR. CAN'T FIND MATERIAL IN MAP WITH INDEX " + std::to_string(index)).c_str());
		}
	}

	MaterialFrameWork MaterialsManager::GetMaterialDesc(size_t index) const
	{
		return m_vMaterials[index < m_vMaterials.size() ? index : 0]->GetMaterialDesc();
	}

	size_t MaterialsManager::GetMaterialSize() const
	{
		return m_vMaterials.size();
	}

	Material* MaterialsManager::GetMaterial(size_t index) const
	{
		return m_vMaterials[index < m_vMaterials.size() ? index : 0].get();
	}

	void MaterialsManager::SetMaterialDesc(std::unique_ptr<Material>& pMaterial, const MaterialFrameWork& materialDesc)
	{
		pMaterial->SetMaterialDesc(materialDesc);
	}

	void MaterialsManager::SetTexture(std::unique_ptr<Material>& pMaterial, std::string& texturePath, TextureType type, ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList)
	{
		pMaterial->SetTexture(texturePath, type, pDevice, pCommandList);
	}

}