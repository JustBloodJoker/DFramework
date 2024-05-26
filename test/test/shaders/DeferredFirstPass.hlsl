#include "Structures.hlsli"

struct VERTEX_OUTPUT
{
    float4 pos : SV_Position;
    float3 worldPos : POSITION0;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
};

struct PIXEL_OUTPUT
{
    float4 positions : SV_TARGET0 ;
    float4 base      : SV_TARGET1 ;
    float4 normals   : SV_TARGET2 ;
};



ConstantBuffer<Matrices> objMatrices : register(b0);
ConstantBuffer<Materials> materialsObject : register(b1);

SamplerState ss : register(s0);
Texture2D tex : register(t0);

VERTEX_OUTPUT VS(VERTEX_INPUT vsIn)
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

    return vsOut;
}

PIXEL_OUTPUT PS(VERTEX_OUTPUT vsOut)
{
    PIXEL_OUTPUT psOut;
    psOut.positions = float4(vsOut.worldPos, 1.0f);
    psOut.base = tex.Sample( ss, vsOut.texCoord );//materialsObject.diffuse;
    clip(psOut.base.a < 0.1f ? -1 : 1);
    psOut.normals = float4(vsOut.normal, 1.0f);



    return psOut;

}
