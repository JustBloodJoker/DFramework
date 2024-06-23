#include "Structures.hlsli"
#include "Utilits.hlsli"

#define NUM_BONES_PER_VEREX 5


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

struct VERTEX_OUTPUT
{
    float4 pos : SV_Position;
    float3 worldPos : POSITION0;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
    uint instance : SV_InstanceID;
};


ConstantBuffer<Matrices> objMatrices : register(b0);

Texture2D tex : register(t0);
SamplerState wraps : register(s0);

StructuredBuffer<matrix> boneMatrices : register(t1);

VERTEX_OUTPUT VS(ANIMVERTEX_INPUT vsIn, uint instance : SV_InstanceID )
{
    VERTEX_OUTPUT vsOut;
    
    matrix ResultWorldMatrix = objMatrices.WorldMatrix;
    for(int i = 0; i < NUM_BONES_PER_VEREX; i++)
    {
        ResultWorldMatrix += boneMatrices[vsIn.IDs[i]] * vsIn.Weight[i];
    }

    vsOut.pos = mul(float4(vsIn.pos, 1.0f), ResultWorldMatrix);
    vsOut.worldPos = vsOut.pos.xyz;
    vsOut.pos = mul(vsOut.pos, objMatrices.ViewMatrix);
    vsOut.pos = mul(vsOut.pos, objMatrices.ProjectionMatrix);
    vsOut.texCoord = vsIn.texCoord;
    vsOut.normal = normalize(vsIn.normal);
    vsOut.tangent = vsIn.tangent;
    vsOut.bitangent = vsIn.bitangent;
    vsOut.instance = instance;

    return vsOut;
}

struct PIXEL_OUTPUT
{
    float4 result : SV_TARGET0 ;
};
PIXEL_OUTPUT PS(VERTEX_OUTPUT vsOut)
{
    PIXEL_OUTPUT psOut;
    psOut.result = tex.Sample(wraps, vsOut.texCoord);
    AlphaClipping(psOut.result.a);
    return psOut;
}
