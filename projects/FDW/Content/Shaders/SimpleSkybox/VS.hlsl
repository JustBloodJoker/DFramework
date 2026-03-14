#include "Utilits.hlsli"
#include "Structures.hlsli"
#include "SameShadersStructs.hlsli"

ConstantBuffer<Matrices> objMatrices : register(b0);

VERTEX_OUTPUT VS(STATICVERTEX_INPUT vsIn)
{
    VERTEX_OUTPUT vsOut;
    vsOut.pos = mul(float4(vsIn.pos, 1.0f), objMatrices.WorldMatrix);
    vsOut.pos = mul(vsOut.pos, objMatrices.ViewMatrix);
    vsOut.pos = mul(vsOut.pos, objMatrices.ProjectionMatrix);
    vsOut.texCoord = vsIn.pos;
    
    vsOut.pos.z = vsOut.pos.w;

    return vsOut;
}