#include "Structures.hlsli"

struct VERTEX_OUTPUT
{
    float4 pos : SV_Position;
    uint instance : SV_InstanceID;
};

struct PIXEL_OUTPUT
{
    float4 result : SV_TARGET0 ;
};

ConstantBuffer<Matrices> objMatrices : register(b0);

struct Particle
{
    float3 position;
    float velocity;
    float4 color;
};

StructuredBuffer<Particle> particles : register(t0);

VERTEX_OUTPUT VS(VERTEX_INPUT vsIn, uint instance : SV_InstanceID )
{
    VERTEX_OUTPUT vsOut;
    float3 tmpPos = vsIn.pos + particles[instance].position;


    vsOut.pos = mul(float4(tmpPos, 1.0f), objMatrices.WorldMatrix);
    
    vsOut.pos = mul(vsOut.pos, objMatrices.ViewMatrix);
    vsOut.pos = mul(vsOut.pos, objMatrices.ProjectionMatrix);

    vsOut.instance = instance;

    return vsOut;
}

PIXEL_OUTPUT PS(VERTEX_OUTPUT vsOut)
{
    PIXEL_OUTPUT psOut;
    psOut.result = particles[vsOut.instance].color;
    return psOut;
}
