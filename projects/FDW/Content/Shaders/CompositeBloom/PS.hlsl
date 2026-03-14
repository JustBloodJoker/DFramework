#include "Utilits.hlsli"
#include "Structures.hlsli"
#include "SameShadersStructs.hlsli"

struct PIXEL_OUTPUT
{
    float4 result : SV_TARGET0;
};

Texture2D SceneTex : register(t0);
Texture2D BloomTex : register(t1);
ConstantBuffer<BloomCompositeData> CompositeData : register(b0);

SamplerState ss : register(s0);

PIXEL_OUTPUT PS(VERTEX_OUTPUT vsOut)
{
    PIXEL_OUTPUT psOut;
    float4 scene = SceneTex.Sample(ss, vsOut.texCoord);
    float3 bloom = BloomTex.Sample(ss, vsOut.texCoord).rgb;
    float3 color = scene.xyz + CompositeData.BloomIntensity * bloom;
    psOut.result = float4(color, scene.w);
    return psOut;
}