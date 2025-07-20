#include "Structures.hlsli"
#include "Utilits.hlsli"
#include "SameShadersStructs.hlsli"

#define NUM_BONES_PER_VEREX 13

struct ANIMVERTEX_INPUT
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
    uint IDs[NUM_BONES_PER_VEREX] : IDS_BONES;
    float Weight[NUM_BONES_PER_VEREX] : WEIGHT_BONES;
};

ConstantBuffer<MeshMatrices> objMatrices : register(b0);
StructuredBuffer<matrix> boneMatrices : register(t9);

VERTEX_OUTPUT VS(ANIMVERTEX_INPUT vsIn, uint Instance : SV_InstanceID)
{
    VERTEX_OUTPUT vsOut;
    
    matrix ResultWorldMatrix = objMatrices.WorldMatrix;
    
    if(objMatrices.IsActiveAnimations) {
        for(int i = 0; i < NUM_BONES_PER_VEREX; i++)
        {
            ResultWorldMatrix += boneMatrices[vsIn.IDs[i]] * vsIn.Weight[i];
        }
    }

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