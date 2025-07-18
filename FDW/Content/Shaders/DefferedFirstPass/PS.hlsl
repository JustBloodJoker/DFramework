#include "Structures.hlsli"
#include "Utilits.hlsli"
#include "SameShadersStructs.hlsli"

#include "ParallaxOcclusionMapping.hlsli"

ConstantBuffer<MeshMatrices> objMatrices : register(b0);
ConstantBuffer<Materials> objMaterials : register(b1);

Texture2D DiffuseTexture : register(t0);
Texture2D NormalTexture : register(t1);
Texture2D RoughnessTexture: register(t2);
Texture2D MetalnessTexture : register(t3);
Texture2D HeightTexture : register(t4);
Texture2D SpecularTexture : register(t5);
Texture2D OpacityTexture : register(t6);
Texture2D BumpTexture : register(t7);
Texture2D EmmisiveTexture : register(t8);

SamplerState wraps : register(s0);

struct PIXEL_OUTPUT
{
    float4 result : SV_TARGET0;
};

PIXEL_OUTPUT PS(VERTEX_OUTPUT vsOut)
{
    const int LoadedTexture[12] = (int[12])objMaterials.LoadedTexture;
    float3 normal = normalize(vsOut.normal);
    float3 tangent = normalize(vsOut.tangent);
    float3 bitangent = normalize(vsOut.bitangent);
    float2 texCoord = vsOut.texCoord;
    float3x3 TBN = float3x3(tangent, bitangent, normal);

    if (LoadedTexture[TEXTURE_HEIGHT_LOAD_FLAG_LOCATION])
    {
        POMInputData inData;
        inData.WorldPosition = vsOut.worldPos;
        inData.CameraPosition = objMatrices.CameraPosition;
        inData.TBN = TBN;
        inData.HeightScale = objMaterials.heightScale;
        inData.HeightTexture = HeightTexture;
        inData.Sampler = wraps;
        inData.TextureCoords = texCoord;
        texCoord = ParallaxOcclusionMapping(inData);
    }

    if (LoadedTexture[TEXTURE_NORMAL_LOAD_FLAG_LOCATION])
    {
        float3 normalMapSample = NormalTexture.Sample(wraps, texCoord).rgb;
        normalMapSample = normalize(normalMapSample * 2.0f - 1.0f);
        normal = normalize(mul(normalMapSample, TBN));
    }
    else if (LoadedTexture[TEXTURE_BUMP_LOAD_FLAG_LOCATION])
    {
        //todo bump mapping
    }



    PIXEL_OUTPUT psOut;
    if (LoadedTexture[TEXTURE_BASE_LOAD_FLAG_LOCATION])
    {
        psOut.result = DiffuseTexture.Sample(wraps, texCoord);
    } 
    else
    {
        psOut.result = float4(1.0f, 0.0f,0.0f,1.0f);
    }
    AlphaClipping(psOut.result.a);
    return psOut;
}