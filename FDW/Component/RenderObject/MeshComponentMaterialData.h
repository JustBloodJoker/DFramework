#pragma once

#include <pch.h>
#include <D3DFramework/GraphicUtilites/Materials.h>
#include <D3DFramework/Utilites/Serializer/ReflectionImpl.h>

#define LOADED_FLAGS_DATA_SIZE TEXTURE_TYPE_SIZE+1

#define IS_ORM_TEXTURE_FLAG_POS TEXTURE_TYPE_SIZE

struct MeshComponentMaterialData: FD3DW::MaterialFrameWork{
	std::array<int, LOADED_FLAGS_DATA_SIZE> LoadedTexture = { false };

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

	BEGIN_FIELD_REGISTRATION(MeshComponentMaterialData)
		REGISTER_FIELD(Diffuse)
		REGISTER_FIELD(Ambient)
		REGISTER_FIELD(Emissive)
		REGISTER_FIELD(Specular)
		REGISTER_FIELD(Roughness)
		REGISTER_FIELD(Metalness)
		REGISTER_FIELD(SpecularPower)
		REGISTER_FIELD(HeightScale)
	END_FIELD_REGISTRATION();
};
