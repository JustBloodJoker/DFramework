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
StructuredBuffer<LightAtlasMeta> ShadowLightsMeta : register(t11);
ConstantBuffer<ShadowAtlasParams>  ShadowAtlasData : register(b2);


SamplerState ss : register(s0);
SamplerState pointSS : register(s1);



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

LightAtlasMeta GetLightMeta(int id){
    if(ShadowLightsMeta[id].LightIndex==id) return ShadowLightsMeta[id];

    for(int i=0;i<LHelper.LightCount;++i) {
        if(ShadowLightsMeta[i].LightIndex==id) {
            return ShadowLightsMeta[i];
        }
    }

    LightAtlasMeta empty;
    return empty;
}
float GetShadowFactor(in LightStruct light, int id, float2 UV)
{
    if (LHelper.IsShadowImpl == 0) 
        return 1.0f;

    LightAtlasMeta meta = GetLightMeta(id);

    if (UV.x < meta.ScreenMinU || UV.x >= meta.ScreenMaxU || UV.y < meta.ScreenMinV || UV.y >= meta.ScreenMaxV)
        return 0.0f;

    float2 relUV;
    relUV.x = (UV.x - meta.ScreenMinU) / (meta.ScreenMaxU - meta.ScreenMinU);
    relUV.y = (UV.y - meta.ScreenMinV) / (meta.ScreenMaxV - meta.ScreenMinV);

    float2 atlasUV;
    atlasUV.x = (meta.AtlasOffsetX + relUV.x * meta.AtlasWidth) / ShadowAtlasData.AtlasWidth;
    atlasUV.y = (meta.AtlasOffsetY + relUV.y * meta.AtlasHeight) / ShadowAtlasData.AtlasHeight;

    float2 shadowTexelSize = 1.0 / float2(ShadowAtlasData.AtlasWidth, ShadowAtlasData.AtlasHeight);
    
    float3 centerNormal = normalize(GBuffer_Normal.SampleLevel(ss, UV, 0).xyz);
    float3 centerPos    = GBuffer_Position.SampleLevel(ss, UV, 0).xyz;

    float kernelSpread = 1.0f; 
    float sigmaPlane   = 2.0f;
    float sigmaNormal  = 0.5f;

    float totalWeight = 0.0;
    float result = 0.0;

    //Dithering
    float2 pixel = UV * float2(ShadowAtlasData.ScreenWidth, ShadowAtlasData.ScreenHeight);
    float angle = frac(dot(floor(), float2(12.9898, 78.233))) * 6.2831853;
    float s = sin(angle);
    float c = cos(angle);
    float2x2 rot = float2x2(c, -s, s, c);

    [unroll]
    for (int y = -2; y <= 2; y++)
    {
        [unroll]
        for (int x = -2; x <= 2; x++)
        {
            float2 baseOffset = float2(x, y);
            float2 rotatedOffset = mul(rot, baseOffset);
            
            float2 shadowOffsetUV = rotatedOffset * shadowTexelSize * kernelSpread;
            float2 sampleAtlasUV = atlasUV + shadowOffsetUV;

            float shadow = ShadowFactorTexture.SampleLevel(pointSS, sampleAtlasUV, 0).r;

            float2 sampleRelUV;
            sampleRelUV.x = (sampleAtlasUV.x * ShadowAtlasData.AtlasWidth - meta.AtlasOffsetX) / meta.AtlasWidth;
            sampleRelUV.y = (sampleAtlasUV.y * ShadowAtlasData.AtlasHeight - meta.AtlasOffsetY) / meta.AtlasHeight;

            float2 sampleScreenUV;
            sampleScreenUV.x = lerp(meta.ScreenMinU, meta.ScreenMaxU, sampleRelUV.x);
            sampleScreenUV.y = lerp(meta.ScreenMinV, meta.ScreenMaxV, sampleRelUV.y);

            float3 samplePos = GBuffer_Position.SampleLevel(ss, sampleScreenUV, 0).xyz;
            float3 sampleNormal = normalize(GBuffer_Normal.SampleLevel(ss, sampleScreenUV, 0).xyz);

            float normalDiff = 1.0 - saturate(dot(sampleNormal, centerNormal));
            float wNormal = exp(-(normalDiff * normalDiff) / (sigmaNormal * sigmaNormal));

            float planeDist = abs(dot(centerNormal, samplePos - centerPos));
            float wPos = exp(-(planeDist * planeDist) / (sigmaPlane * sigmaPlane));

            float weight = wNormal * wPos;

            result += shadow * weight;
            totalWeight += weight;
        }
    }

    return (totalWeight > 0.0001) ? result / totalWeight : result;
}
float3 ApplyPointLight(in LightStruct light, float2 UV, int id, float3 WorldPos, float3 N, float3 V, float3 albedo, float metallic, float roughness, float3 F0)
{
    float shadowFactor = GetShadowFactor(light, id, UV);
    if(shadowFactor<=0.0f) return float3(0.0f,0.0f,0.0f);

    float3 L = light.Position - WorldPos;
    float distance = length(L);
    L /= distance;

    float angularRadius = light.SourceRadius / distance;
    roughness = sqrt(roughness * roughness + angularRadius * angularRadius);   
    roughness = clamp(roughness, 0.0, 1.0);

    float attenuation = saturate(1.0 - (distance / light.AttenuationRadius));
    attenuation *= attenuation;

    float3 radiance = light.Color * light.Intensity * attenuation;

    float3 resultLight = CalculatePBRLighting(N, V, L, radiance, albedo, metallic, roughness, F0);

    return resultLight * shadowFactor;
}

float3 ApplySpotLight(in LightStruct light, float2 UV, int id, float3 WorldPos, float3 N, float3 V, float3 albedo, float metallic, float roughness, float3 F0)
{
    float shadowFactor = GetShadowFactor(light, id, UV);
    if(shadowFactor<=0.0f) return float3(0.0f,0.0f,0.0f);

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

    float3 resultLight = CalculatePBRLighting(N, V, L, radiance, albedo, metallic, roughness, F0);
    
    return resultLight * shadowFactor;
}

float3 ApplyDirectionalLight(in LightStruct light, float2 UV, int id, float3 WorldPos, float3 N, float3 V, float3 albedo, float metallic, float roughness, float3 F0)
{
    float shadowFactor = GetShadowFactor(light, id, UV);
    if(shadowFactor<=0.0f) return float3(0.0f,0.0f,0.0f);

    float3 L = normalize(-light.Direction);
    float3 radiance = light.Color * light.Intensity;

    float3 resultLight = CalculatePBRLighting(N, V, L, radiance, albedo, metallic, roughness, F0);

    return resultLight * shadowFactor;
}


float3 ApplyRectLight(in LightStruct light, float2 UV, int id, float3 WorldPos, float3 N, float3 V, float3 albedo, float metallic, float roughness, float3 F0) {
    float shadowFactor = GetShadowFactor(light, id, UV);
    if(shadowFactor<=0.0f) return float3(0.0f,0.0f,0.0f);

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

    float3 color = light.Intensity * light.Color * (spec + diffuse*albedo) * shadowFactor;
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
                Lo += ApplyPointLight(light, uv, i, WorldPos, N, V, albedo, metallic, roughness, F0);
                break;
            case LIGHT_SPOT_LIGHT_ENUM_VALUE:
                Lo += ApplySpotLight(light, uv, i, WorldPos, N, V, albedo, metallic, roughness, F0);
                break;
            case LIGHT_DIRECTIONAL_LIGHT_ENUM_VALUE:
                Lo += ApplyDirectionalLight(light, uv, i, WorldPos, N, V, albedo, metallic, roughness, F0);
                break;
            case LIGHT_RECT_LIGHT_ENUM_VALUE:
                Lo += ApplyRectLight(light, uv, i, WorldPos, N, V, albedo, metallic, roughness, F0);
                break;
        }
    }
    
    float4 emissiveTexture = GBuffer_Emissive.Sample(ss, uv);
    float3 emissive = emissiveTexture.rgb;
    float emissiveFactor = emissiveTexture.w;

    float3 color = Lo * ao;
    color += emissive * emissiveFactor;
    
    AlphaClipping(alpha);
    psOut.result = float4(color, alpha);
    return psOut;
}