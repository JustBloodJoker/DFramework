#include "Utilits.hlsli"
#include "Structures.hlsli"
#include "SameShadersStructs.hlsli"

struct PIXEL_OUTPUT
{
    float4 result : SV_TARGET0;
};

TextureCube skyboxTexture : register(t0);
SamplerState skyboxSampler : register(s0);

PIXEL_OUTPUT PS(VERTEX_OUTPUT vsOut)
{
    PIXEL_OUTPUT psOut;
    psOut.result = skyboxTexture.Sample(skyboxSampler, vsOut.texCoord);
    //psOut.result = float4(vsOut.texCoord, 1.0f);
    return psOut;
}
