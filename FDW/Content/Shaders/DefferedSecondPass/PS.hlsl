#include "Utilits.hlsli"
#include "Structures.hlsli"
#include "SameShadersStructs.hlsli"
#include "PBRFunctions.hlsli"
#include "LTCFunctions.hlsli"
#include "ClusteredLightsUtils.hlsli"

struct PIXEL_OUTPUT
{
    float4 result : SV_TARGET0;
};

ConstantBuffer<LightsHelper> LHelper : register(b0);
StructuredBuffer<LightStruct> Lights : register(t0);
StructuredBuffer<Cluster> Clusters : register(t10);
ConstantBuffer<ClusterParamsPS> ClustersData : register(b1);


Texture2D GBuffer_Position : register(t1);      //x         , y         , z                 , empty
Texture2D GBuffer_Normal : register(t2);        //x         , y         , z                 , empty
Texture2D GBuffer_Albedo : register(t3);        //r         , g         , b                 , a
Texture2D GBuffer_Specular : register(t4);      //r         , g         , b                 , specularFactor
Texture2D GBuffer_Emissive : register(t5);      //r         , g         , b                 , emissiveFactor
Texture2D GBuffer_MaterialData : register(t6);  //roughness , metalness , specular power    , AO

Texture2D<float4> LTC_Mat : register(t7);
Texture2D<float2> LTC_Amp : register(t8);

Texture2D ShadowFactorTexture : register(t9);

SamplerState ss : register(s0);



float3 CalculatePBRLighting(
    float3 N, float3 V, float3 L, float3 radiance, float3 albedo, float metallic, float roughness, float3 F0
) {
    float3 H = normalize(V + L);

    float NDF = DistributionGGX(N, H, roughness);
    float G   = GeometrySmith(N, V, L, roughness);
    float3 F  = FresnelSchlick(max(dot(H, V), 0.0), F0);

    float3 kS = F;
    float3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;

    float3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
    float3 specular = numerator / max(denominator, 0.001);

    float NdotL = max(dot(N, L), 0.0);

    return (kD * albedo / PI + specular) * radiance * NdotL;
}


float3 ApplyPointLight(in LightStruct light, float3 WorldPos, float3 N, float3 V, float3 albedo, float metallic, float roughness, float3 F0)
{
    float3 L = light.Position - WorldPos;
    float distance = length(L);
    L /= distance;



    float angularRadius = light.SourceRadius / distance;
    roughness = sqrt(roughness * roughness + angularRadius * angularRadius);   
    roughness = clamp(roughness, 0.0, 1.0);

    float attenuation = saturate(1.0 - (distance / light.AttenuationRadius));
    attenuation *= attenuation;

    float3 radiance = light.Color * light.Intensity * attenuation;

    return CalculatePBRLighting(N, V, L, radiance, albedo, metallic, roughness, F0);
}

float3 ApplySpotLight(in LightStruct light, float3 WorldPos, float3 N, float3 V, float3 albedo, float metallic, float roughness, float3 F0)
{
    float3 L = light.Position - WorldPos;
    float distance = length(L);
    L = L / distance;

    float3 spotDir = normalize(-light.Direction);
    float spotCos = dot(L, spotDir);

    float cosOuter = cos(light.OuterConeAngle);
    float cosInner = cos(light.InnerConeAngle);

    float spotFalloff = saturate((spotCos - cosOuter) / max(cosInner - cosOuter, 0.001));

    float attenuation = saturate(1.0 - (distance / light.AttenuationRadius));
    attenuation *= attenuation;

    float3 radiance = light.Color * light.Intensity * attenuation * spotFalloff;

    return CalculatePBRLighting(N, V, L, radiance, albedo, metallic, roughness, F0);
}

float3 ApplyDirectionalLight(in LightStruct light, float3 WorldPos, float3 N, float3 V, float3 albedo, float metallic, float roughness, float3 F0)
{
    float3 L = normalize(-light.Direction);
    float3 radiance = light.Color * light.Intensity;
    return CalculatePBRLighting(N, V, L, radiance, albedo, metallic, roughness, F0);
}


float3 ApplyRectLight(in LightStruct light, float3 WorldPos, float3 N, float3 V, float3 albedo, float metallic, float roughness, float3 F0) {
    float3 points[4];
    InitRectPoints(light, points);
    
    float NdotV = clamp(dot(N, V), 0.0, 1.0);
    roughness = max(0.1, roughness);
    float2 uv = float2(roughness, sqrt(1.0 - NdotV));
    uv = uv*LUT_SCALE + LUT_BIAS;
    
    float4 ltc1 = LTC_Mat.SampleLevel(ss, uv, 0);
    float2 ltc2 = LTC_Amp.SampleLevel(ss, uv, 0);

	float3x3 minV = float3x3(
	   float3(ltc1.x, 0, ltc1.y),
	   float3(0, 1, 0),
	   float3(ltc1.z, 0, ltc1.w)
	);
	
    minV = transpose(minV);
    
    float3 spec = LTCEvaluate(N, V, WorldPos, minV, points);
    
    spec *= F0 * ltc2.x + (1.0 - F0) * ltc2.y;

    float3 diffuse = LTCEvaluate(N, V, WorldPos, float3x3(1, 0, 0, 0, 1, 0, 0, 0, 1), points);

    float3 color = light.Intensity * light.Color * (spec + diffuse*albedo);
    return color;
}

PIXEL_OUTPUT PS(VERTEX_OUTPUT vsOut)
{
    PIXEL_OUTPUT psOut;

    float2 uv = vsOut.texCoord;
    float3 N = normalize(GBuffer_Normal.Sample(ss, uv).rgb);
    float3 WorldPos = GBuffer_Position.Sample(ss, uv).rgb;
    float3 V = normalize(LHelper.CameraPos - WorldPos);

    float4 albedoOut = GBuffer_Albedo.Sample(ss, uv);
    float3 albedo = albedoOut.rgb;
    float alpha = albedoOut.a;

    float shadowFactor = ShadowFactorTexture.Sample(ss, uv).r;

    float4 matData = GBuffer_MaterialData.Sample(ss, uv);
    float roughness = matData.r;
    float metallic = matData.g;
    float ao = matData.a;

    float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);

    float3 posVS = mul(float4(WorldPos, 1.0), ClustersData.ViewMatrix).xyz;
    float  viewZAbs = abs(posVS.z);
    float2 fragCoord = uv;
    uint tileIndex = ComputeClusterIndex(fragCoord, viewZAbs, ClustersData);
    uint lightCount = Clusters[tileIndex].Count;
    
    float3 Lo = float3(0.0, 0.0, 0.0);
    for (int i = 0; i < lightCount; ++i) {
        
        uint lightIndex = Clusters[tileIndex].LightIndices[i];
        LightStruct light = Lights[lightIndex];

        switch (light.LightType) {
            case LIGHT_POINT_LIGHT_ENUM_VALUE:
                Lo += ApplyPointLight(light, WorldPos, N, V, albedo, metallic, roughness, F0);
                break;
            case LIGHT_SPOT_LIGHT_ENUM_VALUE:
                Lo += ApplySpotLight(light, WorldPos, N, V, albedo, metallic, roughness, F0);
                break;
            case LIGHT_DIRECTIONAL_LIGHT_ENUM_VALUE:
                Lo += ApplyDirectionalLight(light, WorldPos, N, V, albedo, metallic, roughness, F0);
                break;
            case LIGHT_RECT_LIGHT_ENUM_VALUE:
                Lo += ApplyRectLight(light, WorldPos, N, V, albedo, metallic, roughness, F0);
                break;
        }
    }
    
    float4 emissiveTexture = GBuffer_Emissive.Sample(ss, uv);
    float3 emissive = emissiveTexture.rgb;
    float emissiveFactor = emissiveTexture.w;

    float3 color = Lo * ao;

    float shadowFactorRes = LHelper.IsShadowImpl==1 ? shadowFactor : 1.0;
    color *= shadowFactorRes;
    color += emissive * emissiveFactor;
    
    AlphaClipping(alpha);
    psOut.result = float4(color, alpha);
    return psOut;
}