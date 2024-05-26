#include "../pch.h"
#include "MaterialsMananger.h"


namespace FDW
{



	

	void MaterialsManager::AddMaterial()
	{
		materials.push_back(std::make_unique<Material>());
	}

	void MaterialsManager::SetTexture(std::string& texturePath, TEXTURE_TYPE type, ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, const std::size_t index)
	{

		if (index < materials.size() && index >= 0)
		{
			CONSOLE_MESSAGE(std::string("SET TEXTURE TO THE MATERIAL WITH INDEX " + std::to_string(index)).c_str());
			SetTexture(materials[index], texturePath, type, pDevice, pCommandList);
		}
		else if(index == -1)
		{
			CONSOLE_MESSAGE(std::string("SET TEXTURE TO THE MATERIAL WITH INDEX " + std::to_string(materials.size() - 1)).c_str());
			SetTexture(*materials.rbegin(), texturePath, type, pDevice, pCommandList);
		}
		else
		{
			CONSOLE_MESSAGE(std::string("SET TEXTURE ERROR. CAN'T FIND MATERIAL WITH INDEX " + std::to_string(index)).c_str());
		}
	}

	void MaterialsManager::SetMaterialDesc(const MaterialFrameWork& materialDesc, const std::size_t index)
	{
		if (index < materials.size() && index >= 0)
		{
			CONSOLE_MESSAGE(std::string("SET MATERIALDESC TO THE MATERIAL WITH INDEX " + std::to_string(index)).c_str());
			SetMaterialDesc(materials[index], materialDesc);
		}
		else if (index == -1)
		{
			CONSOLE_MESSAGE(std::string("SET MATERIALDESC TO THE MATERIAL WITH INDEX " + std::to_string(materials.size() - 1)).c_str());
			SetMaterialDesc(*materials.rbegin(), materialDesc);
		}
		else
		{
			CONSOLE_MESSAGE(std::string("SET MATERIALDESC ERROR. CAN'T FIND MATERIAL IN MAP WITH INDEX " + std::to_string(index)).c_str());
		}
	}

	MaterialFrameWork MaterialsManager::GetMaterialDesc(size_t index) const
	{
		return materials[index < materials.size() ? index : 0]->GetMaterialDesc();
	}

	size_t MaterialsManager::GetMaterialSize() const
	{
		return materials.size();
	}

	Material* MaterialsManager::GetMaterial(size_t index) const
	{
		return materials[index < materials.size() ? index : 0].get();
	}

	void MaterialsManager::SetMaterialDesc(std::unique_ptr<Material>& pMaterial, const MaterialFrameWork& materialDesc)
	{
		pMaterial->SetMaterialDesc(materialDesc);
	}

	void MaterialsManager::SetTexture(std::unique_ptr<Material>& pMaterial, std::string& texturePath, TEXTURE_TYPE type, ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList)
	{
		pMaterial->SetTexture(texturePath, type, pDevice, pCommandList);
	}

}