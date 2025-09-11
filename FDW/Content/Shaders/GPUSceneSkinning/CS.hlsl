#include "Structures.hlsli"
#include "Utilits.hlsli"

StructuredBuffer<matrix> boneMatrices : register(t1);

struct ANIMVERTEX_INPUT
{
    float3 pos;
    float3 normal;
    float2 texCoord;
    float3 tangent;
    float3 bitangent;
    uint IDs[NUM_BONES_PER_VEREX];
    float Weight[NUM_BONES_PER_VEREX];
};

struct ANIMVERTEX_OUTPUT
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
};

StructuredBuffer<ANIMVERTEX_INPUT> g_InputVertices : register(t0);
RWStructuredBuffer<ANIMVERTEX_OUTPUT> g_OutputVertices : register(u0);

cbuffer CSParams : register(b1)
{
    uint VertexCount;
    uint NumBones;
    uint AnimationEnabled;
    uint _pad3;
}

[numthreads(128, 1, 1)]
void CS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint idx = dispatchThreadID.x;
    if (idx >= VertexCount) return;

    ANIMVERTEX_INPUT vin = g_InputVertices[idx];

    matrix skinMatrix = matrix(
        1,0,0,0,
        0,1,0,0,
        0,0,1,0,
        0,0,0,1
    );
    if(AnimationEnabled==1){
        float totalWeight = 0.0f;

        [unroll]
        for (int i = 0; i < NUM_BONES_PER_VEREX; ++i)
        {
            float w = vin.Weight[i];
            if (w > 0.0f)
            {
                uint boneIndex = vin.IDs[i];
                if (NumBones == 0 || boneIndex < NumBones)
                {
                    skinMatrix += boneMatrices[boneIndex] * w;
                }
                totalWeight += w;
            }
        }
        if (totalWeight == 0.0f)
        {
            skinMatrix = matrix(1,0,0,0,  0,1,0,0,  0,0,1,0,  0,0,0,1);
        }
    }
    
    float4 skinnedPos4 = mul(float4(vin.pos, 1.0f), skinMatrix);

    float3x3 skinMat3 = (float3x3)skinMatrix;

    float3 skinnedNormal = normalize(mul(vin.normal, skinMat3));
    float3 skinnedTangent = normalize(mul(vin.tangent, skinMat3));
    float3 skinnedBitangent = normalize(mul(vin.bitangent, skinMat3));

    ANIMVERTEX_OUTPUT outv;
    outv.pos = skinnedPos4.xyz;
    outv.normal = skinnedNormal;
    outv.texCoord = vin.texCoord;
    outv.tangent = skinnedTangent;
    outv.bitangent = skinnedBitangent;
    
    g_OutputVertices[idx] = outv;
}
