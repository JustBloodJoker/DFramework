#include "Structures.hlsli"
#include "Utilits.hlsli"
#include "SameShadersStructs.hlsli" 

ConstantBuffer<Matrices> objMatrices : register(b0);

VERTEX_OUTPUT VS(STATICVERTEX_INPUT vsIn, uint Instance : SV_InstanceID)
{
    VERTEX_OUTPUT vsOut;

    vsOut.pos = mul(float4(vsIn.pos, 1.0f), objMatrices.WorldMatrix);
    vsOut.worldPos = vsOut.pos.xyz;
    vsOut.pos = mul(vsOut.pos, objMatrices.ViewMatrix);
    vsOut.pos = mul(vsOut.pos, objMatrices.ProjectionMatrix);
    vsOut.texCoord = vsIn.texCoord;
    vsOut.normal = normalize(vsIn.normal);
    vsOut.tangent = vsIn.tangent;
    vsOut.bitangent = vsIn.bitangent;
    vsOut.instance = Instance;

    return vsOut;
}