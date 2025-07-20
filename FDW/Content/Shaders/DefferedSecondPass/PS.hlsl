#include "Utilits.hlsli"
#include "Structures.hlsli"
#include "SameShadersStructs.hlsli"
#include "PBRFunctions.hlsli"

struct PIXEL_OUTPUT
{
    float4 result : SV_TARGET0;
};

ConstantBuffer<LightsHelper> LHelper : register(b0);
StructuredBuffer<LightStruct> Lights : register(t0);

Texture2D GBuffer_Position : register(t1);      //x         , y         , z                 , empty
Texture2D GBuffer_Normal : register(t2);        //x         , y         , z                 , empty
Texture2D GBuffer_Albedo : register(t3);        //r         , g         , b                 , a
Texture2D GBuffer_Specular : register(t4);      //r         , g         , b                 , specularFactor
Texture2D GBuffer_Emissive : register(t5);      //r         , g         , b                 , empty
Texture2D GBuffer_MaterialData : register(t6);  //roughness , metalness , specular power    , AO

SamplerState ss : register(s0);

PIXEL_OUTPUT PS(VERTEX_OUTPUT vsOut)
{
    PIXEL_OUTPUT psOut;
    float3 N = GBuffer_Normal.Sample(ss, vsOut.texCoord).rgb;

    float3 WorldPos = GBuffer_Position.Sample(ss, vsOut.texCoord).rgb;

    float3 V = normalize(LHelper.CameraPos - WorldPos);
    
    float4 albedoOut = GBuffer_Albedo.Sample(ss,vsOut.texCoord);
    float3 albedo = albedoOut.rgb;
    float alpha = albedoOut.a;

    float4 matData = GBuffer_MaterialData.Sample(ss, vsOut.texCoord);
    float roughness = matData.r;
    float metallic = matData.g;
    float ao = matData.a;

    float3 F0 = float3(0.04f,0.04f,0.04f); 
    F0 = lerp(F0, albedo, metallic);

    float3 Lo = float3(0.0f,0.0f,0.0f);
    for(int i = 0; i < LHelper.LightCount; ++i) 
    {
        float3 L = normalize(Lights[i].Position - WorldPos);
        float3 H = normalize(V + L);
        float distance = length(Lights[i].Position - WorldPos);
        float attenuation = 1.0 / (distance * distance);
        float3 radiance = Lights[i].Color * attenuation;        
        
        float NDF = DistributionGGX(N, H, roughness);        
        float G = GeometrySmith(N, V, L, roughness);      
        float3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);       
        
        float3 kS = F;
        float3 kD = float3(1.0,1.0,1.0) - kS;
        kD *= 1.0 - metallic;	  
        
        float3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
        float3 specular = numerator / max(denominator, 0.001);  

        float NdotL = max(dot(N, L), 0.0);                
        Lo += (kD * albedo / PI + specular) * radiance * NdotL; 
    }   
  
    float3 ambient = float3(0.03,0.03,0.03) * albedo * ao;
    float3 color = ambient + Lo;
	
    color = color / (color + float3(1.0,1.0,1.0));
    color = pow(color, float3(1.0/2.2,1.0/2.2,1.0/2.2));  
    
    AlphaClipping(alpha);

    psOut.result = float4(color, alpha);
    return psOut;
}