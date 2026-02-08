#pragma once

#include <pch.h>
#include <D3DFramework/GraphicUtilites/Materials.h>
#include <WinWindow/Utils/Reflection/Reflection.h>

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

    REFLECT_STRUCT(MeshComponentMaterialData)
    BEGIN_REFLECT(MeshComponentMaterialData)
        REFLECT_PROPERTY(Diffuse)
        REFLECT_PROPERTY(Ambient)
        REFLECT_PROPERTY(Emissive)
        REFLECT_PROPERTY(Specular)
        REFLECT_PROPERTY(Roughness)
        REFLECT_PROPERTY(Metalness)
        REFLECT_PROPERTY(SpecularPower)
        REFLECT_PROPERTY(HeightScale)
    END_REFLECT(MeshComponentMaterialData)
};
