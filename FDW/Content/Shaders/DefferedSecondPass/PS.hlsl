#include "Utilits.hlsli"
#include "Structures.hlsli"
#include "SameShadersStructs.hlsli"

struct PIXEL_OUTPUT
{
    float4 result : SV_TARGET0;
};

Texture2D screenRTV : register(t0);
SamplerState ss : register(s0);

PIXEL_OUTPUT PS(VERTEX_OUTPUT vsOut)
{
    PIXEL_OUTPUT psOut;
    psOut.result = screenRTV.Sample( ss, vsOut.texCoord );
    clip(psOut.result.a < 0.1f ? -1 : 1);
    return psOut;
}
