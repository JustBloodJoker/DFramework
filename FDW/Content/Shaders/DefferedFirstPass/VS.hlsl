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
    uint IDs[NUM_BONES_PER_VEREX] : IDS_BONES;
    float Weight[NUM_BONES_PER_VEREX] : WEIGHT_BONES;
};

ConstantBuffer<MeshMatrices> objMatrices : register(b0);
StructuredBuffer<matrix> boneMatrices : register(t9, space1);

VERTEX_OUTPUT VS(ANIMVERTEX_INPUT vsIn, uint Instance : SV_InstanceID)
{
    VERTEX_OUTPUT vsOut;
    
    matrix skinMatrix = matrix(1,0,0,0,  0,1,0,0,  0,0,1,0,  0,0,0,1);
    if (objMatrices.IsActiveAnimations==1) 
    {

        float totalWeight = 0.0f;
        for (int i = 0; i < NUM_BONES_PER_VEREX; i++)
        {
            uint boneIndex = vsIn.IDs[i];
            float w = vsIn.Weight[i];
            if (w > 0.0f)
            {
                skinMatrix += boneMatrices[boneIndex] * w;
                totalWeight += w;
            }
        }
        if (totalWeight == 0.0f)
        {
            skinMatrix = matrix(1,0,0,0,  0,1,0,0,  0,0,1,0,  0,0,0,1);
        }
    }

    matrix ResultWorldMatrix = mul(skinMatrix, objMatrices.WorldMatrix);


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