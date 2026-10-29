#pragma once

#include <D3DFramework/GraphicUtilites/Materials.h>
#include <WinWindow/Utils/Reflection/Reflection.h>

#define MATERIAL_TEXTURE_DATA_SIZE (TEXTURE_TYPE_SIZE + 3)
#define MATERIAL_ALPHA_MODE_FLAG_POS TEXTURE_TYPE_SIZE

struct MeshComponentMaterialData: FD3DW::MaterialFrameWork{
    std::array<int, MATERIAL_TEXTURE_DATA_SIZE> LoadedTexture = { false };

    void operator=(const FD3DW::MaterialFrameWork& frameworkData);

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
