#include "Structures.hlsli"

struct VERTEX_OUTPUT
{
    float4 pos : SV_Position;  
    float2 texCoord : TEXCOORD;
};

struct PIXEL_OUTPUT
{
    float4 result : SV_TARGET0;
};

Texture2D screenRTV : register(t0);
SamplerState ss : register(s0);

VERTEX_OUTPUT VS(VERTEX_INPUT vsIn)
{
    VERTEX_OUTPUT vsOut;
    vsOut.pos = float4(vsIn.pos, 1.0f);
    vsOut.texCoord = vsIn.texCoord;
    return vsOut;
}

PIXEL_OUTPUT PS(VERTEX_OUTPUT vsOut)
{
    PIXEL_OUTPUT psOut;
    psOut.result = screenRTV.Sample( ss, vsOut.texCoord );
    clip(psOut.result.a < 0.1f ? -1 : 1);
    return psOut;
}
