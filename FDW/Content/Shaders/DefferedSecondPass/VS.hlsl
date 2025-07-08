#include "Utilits.hlsli"
#include "Structures.hlsli"
#include "SameShadersStructs.hlsli"

VERTEX_OUTPUT VS(VERTEX_INPUT vsIn)
{
    VERTEX_OUTPUT vsOut;
    vsOut.pos = float4(vsIn.pos, 1.0f);
    vsOut.texCoord = vsIn.texCoord;
    return vsOut;
}