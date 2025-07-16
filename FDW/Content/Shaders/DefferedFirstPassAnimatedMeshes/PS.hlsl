#include "Structures.hlsli"
#include "Utilits.hlsli"
#include "SameShadersStructs.hlsli"






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
    PIXEL_OUTPUT psOut;
    psOut.result = DiffuseTexture.Sample(wraps, vsOut.texCoord);
    AlphaClipping(psOut.result.a);
    return psOut;
}