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
StructuredBuffer<Cluster> Clusters : register(t11);
ConstantBuffer<ClusterParamsPS> ClustersData : register(b1);

Texture2D GBuffer_Normal : register(t1);        //x         , y         , z                 , empty
Texture2D GBuffer_Albedo : register(t2);        //r         , g         , b                 , a
Texture2D GBuffer_Specular : register(t3);      //r         , g         , b                 , specularFactor
Texture2D GBuffer_Emissive : register(t4);      //r         , g         , b                 , emissiveFactor
Texture2D GBuffer_MaterialData : register(t5);  //roughness , metalness , specular power    , AO
Texture2D GBuffer_MotionVector : register(t6);  //dtx       , dty

Texture2D DepthBuffer : register(t7);

Texture2D<float4> LTC_Mat : register(t8);
Texture2D<float2> LTC_Amp : register(t9);

Texture2D ShadowFactorTexture : register(t10);
StructuredBuffer<LightAtlasMeta> ShadowLightsMeta : register(t12);
ConstantBuffer<ShadowAtlasParams>  ShadowAtlasData : register(b2);


SamplerState ss : register(s0);
SamplerState linearSS : register(s1);

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

float InterleavedGradientNoise(float2 position_screen)
{
    float3 magic = float3(0.06711056f, 0.00583715f, 52.9829189f);
    return frac(magic.z * frac(dot(position_screen, magic.xy)));
}

float GetLinearDepth(float2 screenUV)
{
    uint w, h;
    DepthBuffer.GetDimensions(w, h);
    int3 texCoord = int3(screenUV * float2(w, h), 0);
    float z_b = DepthBuffer.Load(texCoord).r;
    
    float nearClip = LHelper.ZNear;
    float farClip = LHelper.ZFar;
    return (nearClip * farClip) / (farClip - z_b * (farClip - nearClip));
}

static const float2 PoissonDisk4[4] = {
    float2(-0.5f, -0.5f), float2(0.5f, -0.5f),
    float2(-0.5f,  0.5f), float2(0.5f,  0.5f)
};

float GetShadowFactor(in LightStruct light, int id, float2 UV) 
{
    if (LHelper.IsShadowImpl == 0) return 1.0f;
    LightAtlasMeta meta = GetLightMeta(id);
    
    if (UV.x < meta.ScreenMinU || UV.x >= meta.ScreenMaxU || 
        UV.y < meta.ScreenMinV || UV.y >= meta.ScreenMaxV)
        return 0.0f;

    float centerDepth = GetLinearDepth(UV);

    float2 relUV = (UV - float2(meta.ScreenMinU, meta.ScreenMinV)) / float2(meta.ScreenMaxU - meta.ScreenMinU, meta.ScreenMaxV - meta.ScreenMinV);

    float2 atlasSize = float2(ShadowAtlasData.AtlasWidth, ShadowAtlasData.AtlasHeight);
    float2 texelSize = 1.0f / atlasSize;
    
    float2 pixelPos = float2( (meta.AtlasOffsetX + relUV.x * meta.AtlasWidth), (meta.AtlasOffsetY + relUV.y * meta.AtlasHeight) );
    float2 baseUV = pixelPos * texelSize;
    
    float2 minUV = float2(meta.AtlasOffsetX, meta.AtlasOffsetY) * texelSize + (0.5f * texelSize);
    float2 maxUV = float2(meta.AtlasOffsetX + meta.AtlasWidth, meta.AtlasOffsetY + meta.AtlasHeight) * texelSize - (0.5f * texelSize);

    uint w, h;
    DepthBuffer.GetDimensions(w, h);
    float2 screenPixel = UV * float2(w, h);
    
    int ffidx = LHelper.FrameIndex;
    float2 noiseOffset = float2( frac(float(ffidx) * 0.6180339887f), frac(float(ffidx) * 0.7548776662f) ) * 100.0f;

    float2 animatedScreenPixel = screenPixel + noiseOffset;
    float noise = InterleavedGradientNoise(animatedScreenPixel);

    float angle = noise * PI * 2;
    float s, c;
    sincos(angle, s, c);
    float2x2 rotMat = float2x2(c, -s, s, c);

    float shadowSum = 0.0f;
    float weightSum = 0.0f;
    
    float filterRadius = ShadowAtlasData.FilterRadius; 
    float depthRejectionSharpness = ShadowAtlasData.DepthRejectionSharpness; 

    [unroll]
    for (int i = 0; i < 4; i++) 
    {
        float2 rotatedOffset = mul(rotMat, PoissonDisk4[i]);
        float2 sampleUV = clamp(baseUV + rotatedOffset * texelSize * filterRadius, minUV, maxUV);
        
        float shadow = ShadowFactorTexture.SampleLevel(linearSS, sampleUV, 0).r;

        float2 tileUV = (sampleUV * atlasSize - float2(meta.AtlasOffsetX, meta.AtlasOffsetY)) / float2(meta.AtlasWidth, meta.AtlasHeight);
        float2 screenUV = lerp(float2(meta.ScreenMinU, meta.ScreenMinV), float2(meta.ScreenMaxU, meta.ScreenMaxV), tileUV);

        float sampleDepth = GetLinearDepth(screenUV);
        
        float depthDiff = abs(centerDepth - sampleDepth);
        float weight = exp(-depthDiff * depthDiff * depthRejectionSharpness);

        shadowSum += shadow * weight;
        weightSum += weight;
    }

    if (weightSum < 0.001f) {
        return ShadowFactorTexture.SampleLevel(linearSS, baseUV, 0).r;
    }

    float result = shadowSum / weightSum;
    
    float blackLevel = ShadowAtlasData.BlackLevel;
    return saturate((result - blackLevel) / (1.0f - blackLevel));
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

    float3 WorldPos = ReconstructWorldPosition(uv,  DepthBuffer.Sample(ss, uv).r, LHelper.InverseViewProjectionMatrix);

    float4 nn = GBuffer_Normal.Sample(ss, uv).rgba;
    if(nn.a==3.0f) {
        psOut.result = float4(0,0,0,3.0f);
        return psOut;
    }

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
