#include "Structures.hlsli"
#include "Utilits.hlsli"
#include "SameShadersStructs.hlsli"

#include "ParallaxOcclusionMapping.hlsli"
#include "NormalMapping.hlsli"

ConstantBuffer<MeshMatrices> objMatrices : register(b0);
ConstantBuffer<Materials> objMaterials : register(b1);

Texture2D GlobalTextures[] : register(t0, space0);

SamplerState wraps : register(s0);

struct PIXEL_OUTPUT
{
    float4 Position : SV_TARGET0;
    float4 Normal : SV_TARGET1;
    float4 Albedo : SV_TARGET2;
    float4 Specular : SV_TARGET3;
    float4 Emissive : SV_TARGET4;
    float4 MaterialData : SV_TARGET5;
};

// TODO:    TBN calcs remove to vertex shader
PIXEL_OUTPUT PS(VERTEX_OUTPUT vsOut)
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

    float roughness = objMaterials.roughness;
    if (LoadedTexture[TEXTURE_ROUGHNESS_LOAD_FLAG_LOCATION]!=-1)
    {
        if(LoadedTexture[TEXTURE_ORM_TEXTURE_TYPE_FLAG_LOCATION])
        {
            roughness = GlobalTextures[LoadedTexture[TEXTURE_ROUGHNESS_LOAD_FLAG_LOCATION]].Sample(wraps, texCoord).g;
        } else
        {
            roughness = GlobalTextures[LoadedTexture[TEXTURE_ROUGHNESS_LOAD_FLAG_LOCATION]].Sample(wraps, texCoord).r;
        }
    }

    float metalness = objMaterials.metalness;
    if (LoadedTexture[TEXTURE_METALNESS_LOAD_FLAG_LOCATION]!=-1)
    {
        if(LoadedTexture[TEXTURE_ORM_TEXTURE_TYPE_FLAG_LOCATION])
        {
            metalness = GlobalTextures[LoadedTexture[TEXTURE_METALNESS_LOAD_FLAG_LOCATION]].Sample(wraps, texCoord).b;
        } else
        {
            metalness = GlobalTextures[LoadedTexture[TEXTURE_METALNESS_LOAD_FLAG_LOCATION]].Sample(wraps, texCoord).r;
        }
    }

    float4 specular = objMaterials.specular;
    if (LoadedTexture[TEXTURE_SPECULAR_LOAD_FLAG_LOCATION]!=-1)
    {
        specular.w = GlobalTextures[LoadedTexture[TEXTURE_SPECULAR_LOAD_FLAG_LOCATION]].Sample(wraps, texCoord).r;
    }
    
    float4 emissive = objMaterials.emissive;
    if (LoadedTexture[TEXTURE_EMISSIVE_LOAD_FLAG_LOCATION]!=-1)
    {
        emissive.rgb = GlobalTextures[LoadedTexture[TEXTURE_EMISSIVE_LOAD_FLAG_LOCATION]].Sample(wraps, texCoord).rgb;
    }
    
    float ao = 1.0f;
    if (LoadedTexture[TEXTURE_AMBIENT_LOAD_FLAG_LOCATION]!=-1)
    {
        ao = GlobalTextures[LoadedTexture[TEXTURE_AMBIENT_LOAD_FLAG_LOCATION]].Sample(wraps, texCoord).r;
    }

    AlphaClipping(albedo.a);

    PIXEL_OUTPUT psOut;
    psOut.Position      = float4(vsOut.worldPos, 1.0f);
    psOut.Normal        = float4(normal, 1.0f);
    psOut.Albedo        = albedo;
    psOut.Specular      = specular;
    psOut.Emissive      = emissive;
    psOut.MaterialData  = float4(roughness, metalness, objMaterials.specularPower, ao);

    return psOut;
}