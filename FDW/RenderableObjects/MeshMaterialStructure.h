#pragma once

#include <pch.h>
#include <D3DFramework/GraphicUtilites/Materials.h>

struct MeshMaterialStructure : FD3DW::MaterialFrameWork {
	int LoadedTexture[FD3DW::TextureType::SIZE];

	void operator=(const FD3DW::MaterialFrameWork& frameworkData) {
		Diffuse = frameworkData.Diffuse;
		Ambient = frameworkData.Ambient;
		Emissive = frameworkData.Emissive;
		Specular = frameworkData.Specular;
		Roughness = frameworkData.Roughness;
		Metalness = frameworkData.Metalness;
		SpecularPower = frameworkData.SpecularPower;
		HeightScale = frameworkData.HeightScale;
	}
};

