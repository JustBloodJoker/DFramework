#include "Utilits.hlsli"
#include "Structures.hlsli"
#include "SameShadersStructs.hlsli"

struct PIXEL_OUTPUT
{
    float4 result : SV_TARGET0;
};

ConstantBuffer<LightsHelper> LightsHelper : register(b0);
StructuredBuffer<LightStruct> Lights : register(t0);

Texture2D GBuffer_Position : register(t1);      //x         , y         , z                 , empty
Texture2D GBuffer_Normal : register(t2);        //x         , y         , z                 , empty
Texture2D GBuffer_Albedo : register(t3);        //r         , g         , b                 , a
Texture2D GBuffer_Specular : register(t4);      //r         , g         , b                 , specularFactor
Texture2D GBuffer_Emissive : register(t5);      //r         , g         , b                 , empty
Texture2D GBuffer_MaterialData : register(t6);  //roughness , metalness , specular power    , empty

SamplerState ss : register(s0);

PIXEL_OUTPUT PS(VERTEX_OUTPUT vsOut)
{
    PIXEL_OUTPUT psOut;
    psOut.result = GBuffer_Albedo.Sample( ss, vsOut.texCoord );
    clip(psOut.result.a < 0.1f ? -1 : 1);
    return psOut;
}
