#include "Structures.hlsli"

struct VERTEX_OUTPUT
{
    float4 pos : SV_Position;  
    float2 texCoord : TEXCOORD;
};

struct PIXEL_OUTPUT
{
    float4 result : SV_TARGET0 ;
};

SamplerState ss : register(s0);
Texture2D texPos : register(t0);
Texture2D texBase : register(t1);
Texture2D texNormals : register(t2);

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
    psOut.result = texBase.Sample( ss, vsOut.texCoord );//materialsObject.diffuse;
    clip(psOut.result.a < 0.1f ? -1 : 1);
    return psOut;
}
