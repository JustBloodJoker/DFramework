#include "Structures.hlsli"
#include "Utilits.hlsli"
#include "SameShadersStructs.hlsli"

Texture2D tex : register(t0);
SamplerState wraps : register(s0);

struct PIXEL_OUTPUT
{
    float4 result : SV_TARGET0;
};

PIXEL_OUTPUT PS(VERTEX_OUTPUT vsOut)
{
    PIXEL_OUTPUT psOut;
    psOut.result = tex.Sample(wraps, vsOut.texCoord);
    AlphaClipping(psOut.result.a);
    return psOut;
}