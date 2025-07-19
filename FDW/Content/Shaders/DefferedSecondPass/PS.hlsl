#include "Utilits.hlsli"
#include "Structures.hlsli"
#include "SameShadersStructs.hlsli"

struct PIXEL_OUTPUT
{
    float4 result : SV_TARGET0;
};

Texture2D GBuffer_Position : register(t0);
Texture2D GBuffer_Normal : register(t1);
Texture2D GBuffer_Albedo : register(t2);
Texture2D GBuffer_Specular : register(t3);
Texture2D GBuffer_Emissive : register(t4);
Texture2D GBuffer_MaterialData : register(t5);

SamplerState ss : register(s0);

PIXEL_OUTPUT PS(VERTEX_OUTPUT vsOut)
{
    PIXEL_OUTPUT psOut;
    psOut.result = GBuffer_Albedo.Sample( ss, vsOut.texCoord );
    clip(psOut.result.a < 0.1f ? -1 : 1);
    return psOut;
}
