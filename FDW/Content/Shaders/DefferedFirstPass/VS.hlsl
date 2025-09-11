#include "Structures.hlsli"
#include "Utilits.hlsli"
#include "SameShadersStructs.hlsli"


struct ANIMVERTEX_INPUT
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
};

ConstantBuffer<MeshMatrices> objMatrices : register(b0);
StructuredBuffer<matrix> boneMatrices : register(t9, space1);

VERTEX_OUTPUT VS(ANIMVERTEX_INPUT vsIn, uint Instance : SV_InstanceID)
{
    VERTEX_OUTPUT vsOut;
    
    matrix ResultWorldMatrix = objMatrices.WorldMatrix;
    vsOut.pos = mul(float4(vsIn.pos, 1.0f), ResultWorldMatrix);
    vsOut.worldPos = vsOut.pos.xyz;
    vsOut.pos = mul(vsOut.pos, objMatrices.ViewMatrix);
    vsOut.pos = mul(vsOut.pos, objMatrices.ProjectionMatrix);
    vsOut.texCoord = vsIn.texCoord;

    vsOut.normal = normalize(mul(vsIn.normal, (float3x3)ResultWorldMatrix));
    vsOut.tangent = normalize(mul(vsIn.tangent, (float3x3)ResultWorldMatrix)); 
    vsOut.bitangent = normalize(mul(vsIn.bitangent, (float3x3)ResultWorldMatrix));
    
    vsOut.instance = Instance;

    return vsOut;
}