#include "Structures.hlsli"
#include "Utilits.hlsli"
#include "SameShadersStructs.hlsli"

#include "ParallaxOcclusionMapping.hlsli"
#include "NormalMapping.hlsli"

ConstantBuffer<MeshMatrices> objMatrices : register(b0);
ConstantBuffer<Materials> objMaterials : register(b1);

Texture2D GlobalTextures[] : register(t0, space0);

SamplerState wraps : register(s0);

// TODO:    TBN calcs remove to vertex shader
float4 PS(VERTEX_OUTPUT vsOut) : SV_TARGET0
{
    const int LoadedTexture[12] = (int[12])objMaterials.LoadedTexture;
    float3 normal = normalize(vsOut.normal);
    float3 tangent = normalize(vsOut.tangent);
    float3 bitangent = normalize(vsOut.bitangent);
    float2 texCoord = vsOut.texCoord;
    float3x3 TBN = float3x3(normalize(tangent),
                        normalize(bitangent),
                        normalize(normal));

    if (LoadedTexture[TEXTURE_HEIGHT_LOAD_FLAG_LOCATION]!=-1)
    {
        POMInputData inData;
        inData.WorldPosition = vsOut.worldPos;
        inData.CameraPosition = objMatrices.CameraPosition;
        inData.TBN = TBN;
        inData.HeightScale = objMaterials.heightScale;
        inData.HeightTexture = GlobalTextures[LoadedTexture[TEXTURE_HEIGHT_LOAD_FLAG_LOCATION]];
        inData.Sampler = wraps;
        inData.TextureCoords = texCoord;
        texCoord = ParallaxOcclusionMapping(inData);
    }

    if (LoadedTexture[TEXTURE_NORMAL_LOAD_FLAG_LOCATION]!=-1)
    {
        NMInputData inData;
        inData.TextureCoords = texCoord;
        inData.TBN = TBN;
        inData.NormalTexture = GlobalTextures[LoadedTexture[TEXTURE_NORMAL_LOAD_FLAG_LOCATION]];
        inData.Sampler = wraps;
        normal = NormalMapping(inData);
    }

    float4 albedo = objMaterials.diffuse;
    if (LoadedTexture[TEXTURE_BASE_LOAD_FLAG_LOCATION]!=-1)
    {
        albedo = GlobalTextures[LoadedTexture[TEXTURE_BASE_LOAD_FLAG_LOCATION]].Sample(wraps, texCoord);
    }

    if (LoadedTexture[MATERIAL_ALPHA_MODE_FLAG_LOCATION] != MATERIAL_ALPHA_MODE_OPAQUE)
    {
        AlphaClipping(albedo.a);
    }

    return float4(normal, 0.0f);
}
