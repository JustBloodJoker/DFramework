#include "Utilits.hlsli"
#include "Structures.hlsli"
#include "SameShadersStructs.hlsli"

struct PIXEL_OUTPUT
{
    float4 result : SV_TARGET0;
};

Texture2D BloomTex : register(t0);
ConstantBuffer<BlurParams> BlurData : register(b0);

SamplerState ss : register(s0);

static const float Weights[9] = {0.05, 0.09, 0.12, 0.15, 0.18, 0.15, 0.12, 0.09, 0.05};

PIXEL_OUTPUT PS(VERTEX_OUTPUT vsOut)
{
    PIXEL_OUTPUT psOut;
    float3 result = 0;
    for (int i = -4; i <= 4; i++)
    {
        float2 offset = BlurData.Horizontal ? float2(i, 0) * BlurData.TexelSize : float2(0, i) * BlurData.TexelSize;
        result += BloomTex.Sample(ss, vsOut.texCoord + offset).rgb * Weights[i + 4];
    }

    psOut.result = float4(result, 1.0);
    return psOut;
}