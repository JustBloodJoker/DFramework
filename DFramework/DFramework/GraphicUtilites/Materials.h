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
		~Material();

		ID3D12Resource* GetResourceTexture(TEXTURE_TYPE type) const;
		MaterialFrameWork GetMaterialDesc() const;
		void SetMaterialDesc(MaterialFrameWork&& materialDesc);
		void SetMaterialDesc(const MaterialFrameWork& materialDesc);
		void SetTexture(std::string& texturePath, TEXTURE_TYPE type, ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList);

		bool IsHaveTexture(TEXTURE_TYPE type) const;

	private:

		MaterialFrameWork material;
		std::unordered_map<TEXTURE_TYPE, std::unique_ptr<Texture>> textureMap;


	};


}
