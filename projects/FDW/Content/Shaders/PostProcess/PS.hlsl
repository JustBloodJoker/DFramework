#include "Utilits.hlsli"
#include "Structures.hlsli"
#include "SameShadersStructs.hlsli"

struct PIXEL_OUTPUT
{
    float4 result : SV_TARGET0;
};

Texture2D RenderOutput : register(t0);

SamplerState ss : register(s0);

PIXEL_OUTPUT PS(VERTEX_OUTPUT vsOut)
{
    PIXEL_OUTPUT psOut;
    float4 color = RenderOutput.Sample( ss, vsOut.texCoord );
    color.xyz = color.xyz / (color.xyz + float3(1.0, 1.0, 1.0));
    color.xyz = LinearToSRGB(color.xyz);
    psOut.result = color;
    return psOut;
}
