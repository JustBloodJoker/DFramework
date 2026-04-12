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
StructuredBuffer<Cluster> Clusters : register(t15);
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

TextureCube IBL_EnvironmentTexture : register(t11);
Texture2D<float2> IBL_BRDF_LUT : register(t12); //x = scale, y = bias (split-sum BRDF LUT)
TextureCube IBL_IrradianceTexture : register(t13); //preconvolved diffuse irradiance
TextureCube IBL_PrefilteredTexture : register(t14); //GGX prefiltered environment for specular IBL

StructuredBuffer<LightAtlasMeta> ShadowLightsMeta : register(t16);
ConstantBuffer<ShadowAtlasParams>  ShadowAtlasData : register(b2);

SamplerState ss : register(s0);
SamplerState linearSS : register(s1);

static const float3 SELECTION_OUTLINE_COLOR = float3(1.0f, 0.55f, 0.05f);

bool IsSelectionMaskGeometry(float alphaValue) {
    return alphaValue < 2.5f;
}

bool IsSelectionMaskSelected(float alphaValue) {
    return IsSelectionMaskGeometry(alphaValue) && alphaValue > 0.5f;
}

float ComputeSelectionOutlineFactor(float2 uv) {
    uint w, h;
    GBuffer_Normal.GetDimensions(w, h);
    if(w == 0 || h == 0) return 0.0f;

    float centerAlpha = GBuffer_Normal.Sample(ss, uv).a;
    if(!IsSelectionMaskGeometry(centerAlpha)) return 0.0f;

    bool centerSelected = IsSelectionMaskSelected(centerAlpha);
    float2 texel = 1.0f / float2(w, h);

    float outline = 0.0f;
    [unroll]
    for(int y = -1; y <= 1; ++y) {
        [unroll]
        for(int x = -1; x <= 1; ++x) {
            if(x == 0 && y == 0) continue;

            float2 sampleUV = saturate(uv + float2(x, y) * texel);
            float sampleAlpha = GBuffer_Normal.Sample(ss, sampleUV).a;
            bool sampleSelected = IsSelectionMaskSelected(sampleAlpha);
            if(centerSelected != sampleSelected) {
                outline = 1.0f;
            }
        }
    }

    return outline;
}

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

float3 CalculateImageBasedLighting(float3 N, float3 V, float3 albedo, float metallic, float roughness, float ao, float3 F0) {
    float NdotV = saturate(dot(N, V));
    float3 kS = FresnelSchlickRoughness(NdotV, F0, roughness);
    float3 kD = (1.0 - kS) * (1.0 - metallic);

    float3 irradiance = IBL_IrradianceTexture.SampleLevel(ss, N, 0.0f).rgb;
    float3 diffuse = irradiance * albedo;

    float3 R = reflect(-V, N);
    
    uint prefilteredWidth, prefilteredHeight, prefilteredMipCount;

    IBL_PrefilteredTexture.GetDimensions(0u, prefilteredWidth, prefilteredHeight, prefilteredMipCount);
    float3 prefilteredColor = IBL_EnvironmentTexture.SampleLevel(ss, R, 0.0f).rgb;
    if (prefilteredMipCount > 0u) {
        float maxReflectionMip = min(LHelper.IBLMaxReflectionMip, max(float(prefilteredMipCount) - 1.0f, 0.0f));
        float mipLevel = saturate(roughness) * maxReflectionMip;
        prefilteredColor = IBL_PrefilteredTexture.SampleLevel(ss, R, mipLevel).rgb;
    }
    float2 envBRDF = IBL_BRDF_LUT.SampleLevel(linearSS, float2(NdotV, saturate(roughness)), 0.0f).rg;
    float3 specular = prefilteredColor * (kS * envBRDF.x + envBRDF.y);

    float3 diffuseTerm = kD * diffuse * LHelper.IBLDiffuseIntensity;
    float3 specularTerm = specular * LHelper.IBLSpecularIntensity;
    return (diffuseTerm + specularTerm) * ao;
}

LightAtlasMeta GetLightMeta(int id){
    LightAtlasMeta empty = (LightAtlasMeta)0;
    if (id < 0 || id >= LHelper.LightCount) return empty;

    if(ShadowLightsMeta[id].LightIndex==id) return ShadowLightsMeta[id];

    for(int i=0;i<LHelper.LightCount;++i) {
        if(ShadowLightsMeta[i].LightIndex==id) {
            return ShadowLightsMeta[i];
        }
    }

    return empty;
}

float InterleavedGradientNoise(float2 position_screen)
{
    float3 magic = float3(0.06711056f, 0.00583715f, 52.9829189f);
    return frac(magic.z * frac(dot(position_screen, magic.xy)));
}

float2 GetAnimatedNoisePixel(float2 screenUV)
{
    uint w, h;
    DepthBuffer.GetDimensions(w, h);
    float2 screenPixel = screenUV * float2(w, h);

    float frameIdx = (float)LHelper.FrameIndex;
    float2 temporalShift = float2(
        frac(frameIdx * 0.6180339887f),
        frac(frameIdx * 0.7548776662f)
    ) * 100.0f;

    return screenPixel + temporalShift;
}

float GetLinearDepth(float2 screenUV)
{
    uint w, h;
    DepthBuffer.GetDimensions(w, h);
    if (w == 0 || h == 0) return 0.0f;

    int2 texCoord2 = int2(screenUV * float2(w, h));
    texCoord2 = clamp(texCoord2, int2(0, 0), int2((int)w - 1, (int)h - 1));
    int3 texCoord = int3(texCoord2, 0);
    float z_b = DepthBuffer.Load(texCoord).r;
    
    float nearClip = LHelper.ZNear;
    float farClip = LHelper.ZFar;
    return (nearClip * farClip) / (farClip - z_b * (farClip - nearClip));
}

float3 GetSurfaceNormal(float2 screenUV)
{
    float3 normal = GBuffer_Normal.SampleLevel(linearSS, saturate(screenUV), 0).xyz;
    float nLen = max(length(normal), 1e-6f);
    return normal / nLen;
}

float2 AtlasUVToScreenUV(in LightAtlasMeta meta, float2 atlasUV, float2 atlasSize)
{
    float2 atlasOffset = float2(meta.AtlasOffsetX, meta.AtlasOffsetY);
    float2 atlasDim = max(float2(meta.AtlasWidth, meta.AtlasHeight), float2(1.0f, 1.0f));
    float2 tileUV = (atlasUV * atlasSize - atlasOffset) / atlasDim;
    tileUV = saturate(tileUV);
    return lerp(float2(meta.ScreenMinU, meta.ScreenMinV), float2(meta.ScreenMaxU, meta.ScreenMaxV), tileUV);
}

float ComputeJointBilateralWeight(float centerDepth, float3 centerNormal, float2 sampleScreenUV)
{
    float sampleDepth = GetLinearDepth(sampleScreenUV);
    float depthDiff = abs(centerDepth - sampleDepth);
    float depthWeight = exp(-depthDiff * depthDiff * ShadowAtlasData.DepthRejectionSharpness);

    float3 sampleNormal = GetSurfaceNormal(sampleScreenUV);
    float normalDiff = 1.0f - saturate(dot(centerNormal, sampleNormal));
    float normalWeight = exp(-normalDiff * normalDiff * ShadowAtlasData.NormalRejectionSharpness);

    return depthWeight * normalWeight;
}

float ApplyBlackLevelAndContrast(float shadowValue)
{
    float blackLevel = saturate(ShadowAtlasData.BlackLevel);
    float normalized = saturate((shadowValue - blackLevel) / max(1.0f - blackLevel, 1e-6f));
    float contrast = max(ShadowAtlasData.ShadowContrast, 0.1f);
    return saturate(pow(normalized, contrast));
}

static const float2 ShadowRing3[3] = {
    float2(1.0f, 0.0f),
    float2(-0.5f, 0.8660254f),
    float2(-0.5f, -0.8660254f)
};

static const float2 PoissonDisk16[16] = {
    float2(-0.94201624f, -0.39906216f),
    float2(0.94558609f, -0.76890725f),
    float2(-0.09418410f, -0.92938870f),
    float2(0.34495938f, 0.29387760f),
    float2(-0.91588581f, 0.45771432f),
    float2(-0.81544232f, -0.87912464f),
    float2(-0.38277543f, 0.27676845f),
    float2(0.97484398f, 0.75648379f),
    float2(0.44323325f, -0.97511554f),
    float2(0.53742981f, -0.47373420f),
    float2(-0.26496911f, -0.41893023f),
    float2(0.79197514f, 0.19090188f),
    float2(-0.24188840f, 0.99706507f),
    float2(-0.81409955f, 0.91437590f),
    float2(0.19984126f, 0.78641367f),
    float2(0.14383161f, -0.14100790f)
};

float SampleShadowPoint(int2 atlasPixel, in LightAtlasMeta meta)
{
    int2 minPixel = int2(meta.AtlasOffsetX, meta.AtlasOffsetY);
    int2 maxPixel = minPixel + int2(meta.AtlasWidth, meta.AtlasHeight) - int2(1, 1);
    atlasPixel = clamp(atlasPixel, minPixel, maxPixel);
    return ShadowFactorTexture.Load(int3(atlasPixel, 0)).r;
}

float SampleShadowBilinear(float2 atlasUV)
{
    return ShadowFactorTexture.SampleLevel(linearSS, atlasUV, 0).r;
}

float ApplyJointBilateralRingFilter(
    in LightAtlasMeta meta,
    float2 baseUV,
    float2 minUV,
    float2 maxUV,
    float2 atlasSize,
    float2 texelSize,
    float centerDepth,
    float3 centerNormal,
    float2 animatedNoisePixel)
{
    float2 jitterNoise = float2(
        InterleavedGradientNoise(animatedNoisePixel + float2(13.1f, 71.7f)),
        InterleavedGradientNoise(animatedNoisePixel + float2(91.7f, 29.3f))
    );
    float2 subPixelJitter = (jitterNoise - float2(0.5f, 0.5f)) * texelSize * 0.75f * ShadowAtlasData.NoiseScale;
    float2 jitteredBaseUV = clamp(baseUV + subPixelJitter, minUV, maxUV);

    float noise = InterleavedGradientNoise(animatedNoisePixel);
    float angle = noise * PI * 2.0f;
    float s, c;
    sincos(angle, s, c);
    float2x2 rotMat = float2x2(c, -s, s, c);

    float shadowSum = 0.0f;
    float weightSum = 0.0f;

    {
        float2 sampleScreenUV = AtlasUVToScreenUV(meta, jitteredBaseUV, atlasSize);
        float weight = ComputeJointBilateralWeight(centerDepth, centerNormal, sampleScreenUV);
        shadowSum += SampleShadowBilinear(jitteredBaseUV) * weight;
        weightSum += weight;
    }

    [unroll]
    for (int i = 0; i < 3; ++i)
    {
        float2 rotatedOffset = mul(rotMat, ShadowRing3[i]);
        float2 sampleUV = clamp(
            jitteredBaseUV + rotatedOffset * texelSize * max(ShadowAtlasData.FilterRadius, 0.01f),
            minUV,
            maxUV
        );

        float2 sampleScreenUV = AtlasUVToScreenUV(meta, sampleUV, atlasSize);
        float weight = ComputeJointBilateralWeight(centerDepth, centerNormal, sampleScreenUV);
        shadowSum += SampleShadowBilinear(sampleUV) * weight;
        weightSum += weight;
    }

    if (weightSum <= 1e-6f) return SampleShadowBilinear(jitteredBaseUV);
    return shadowSum / weightSum;
}

float ApplyKernelPCFFilter(
    in LightAtlasMeta meta,
    float2 baseUV,
    float2 minUV,
    float2 maxUV,
    float2 atlasSize,
    float2 texelSize,
    int inputKernelSize,
    bool useJointBilateral,
    float centerDepth,
    float3 centerNormal)
{
    int kernelSize = max(inputKernelSize, 1);
    int start = -(kernelSize / 2);
    int end = start + kernelSize;

    float sigma = max(1.0f, float(kernelSize) * 0.5f);
    float invTwoSigma2 = 1.0f / (2.0f * sigma * sigma);
    float radiusScale = max(ShadowAtlasData.FilterRadius, 0.01f);

    float shadowSum = 0.0f;
    float weightSum = 0.0f;

    [loop]
    for (int y = start; y < end; ++y)
    {
        [loop]
        for (int x = start; x < end; ++x)
        {
            float2 offset = float2((float)x, (float)y) * texelSize * radiusScale;
            float2 sampleUV = clamp(baseUV + offset, minUV, maxUV);

            float sampleShadow = SampleShadowBilinear(sampleUV);
            float spatialWeight = exp(-(float(x * x + y * y)) * invTwoSigma2);
            float weight = spatialWeight;

            if (useJointBilateral) {
                float2 sampleScreenUV = AtlasUVToScreenUV(meta, sampleUV, atlasSize);
                weight *= ComputeJointBilateralWeight(centerDepth, centerNormal, sampleScreenUV);
            }

            shadowSum += sampleShadow * weight;
            weightSum += weight;
        }
    }

    if (weightSum <= 1e-6f) return SampleShadowBilinear(baseUV);
    return shadowSum / weightSum;
}

float ApplyPoisson16Filter(
    in LightAtlasMeta meta,
    float2 baseUV,
    float2 minUV,
    float2 maxUV,
    float2 atlasSize,
    float2 texelSize,
    bool useJointBilateral,
    float centerDepth,
    float3 centerNormal,
    float2 animatedNoisePixel)
{
    float angle = InterleavedGradientNoise(animatedNoisePixel) * PI * 2.0f;
    float s, c;
    sincos(angle, s, c);
    float2x2 rotMat = float2x2(c, -s, s, c);

    float radiusScale = max(ShadowAtlasData.FilterRadius, 0.01f);
    float noiseScale = ShadowAtlasData.NoiseScale;

    float shadowSum = 0.0f;
    float weightSum = 0.0f;

    [unroll]
    for (int i = 0; i < 16; ++i)
    {
        float2 rotatedOffset = mul(rotMat, PoissonDisk16[i]);
        float jitter = InterleavedGradientNoise(animatedNoisePixel + float2(float(i) * 17.13f, float(i) * 23.71f)) - 0.5f;
        float2 jitterOffset = texelSize * jitter * noiseScale * 0.5f;
        float2 sampleUV = clamp(baseUV + rotatedOffset * texelSize * radiusScale + jitterOffset, minUV, maxUV);

        float sampleShadow = SampleShadowBilinear(sampleUV);
        float weight = 1.0f;

        if (useJointBilateral) {
            float2 sampleScreenUV = AtlasUVToScreenUV(meta, sampleUV, atlasSize);
            weight *= ComputeJointBilateralWeight(centerDepth, centerNormal, sampleScreenUV);
        }

        shadowSum += sampleShadow * weight;
        weightSum += weight;
    }

    if (weightSum <= 1e-6f) return SampleShadowBilinear(baseUV);
    return shadowSum / weightSum;
}

float GetShadowFactor(int id, float2 UV)
{
    if (LHelper.IsShadowImpl == 0) return 1.0f;
    
    LightAtlasMeta meta = GetLightMeta(id);
    if (meta.AtlasWidth == 0 || meta.AtlasHeight == 0) return 1.0f;
    if (meta.ScreenMaxU <= meta.ScreenMinU || meta.ScreenMaxV <= meta.ScreenMinV) return 1.0f;
    if (UV.x < meta.ScreenMinU || UV.x >= meta.ScreenMaxU || UV.y < meta.ScreenMinV || UV.y >= meta.ScreenMaxV)
        return 1.0f;

    float2 atlasSize = float2(max(ShadowAtlasData.AtlasWidth, 1u), max(ShadowAtlasData.AtlasHeight, 1u));
    float2 texelSize = 1.0f / atlasSize;

    float2 screenMinUV = float2(meta.ScreenMinU, meta.ScreenMinV);
    float2 screenMaxUV = float2(meta.ScreenMaxU, meta.ScreenMaxV);
    float2 relUV = (UV - screenMinUV) / max(screenMaxUV - screenMinUV, float2(1e-5f, 1e-5f));
    relUV = saturate(relUV);

    float2 tileSize = float2(meta.AtlasWidth, meta.AtlasHeight);
    float2 minPixel = float2(meta.AtlasOffsetX, meta.AtlasOffsetY);
    float2 maxPixel = minPixel + tileSize - float2(1.0f, 1.0f);
    float2 atlasPixel = clamp(minPixel + relUV * tileSize, minPixel, maxPixel);
    int2 atlasPixelInt = int2(atlasPixel);

    float2 baseUV = (atlasPixel + 0.5f) * texelSize;
    float2 minUV = (minPixel + 0.5f) * texelSize;
    float2 maxUV = (minPixel + tileSize - 0.5f) * texelSize;

    float centerDepth = GetLinearDepth(UV);
    float3 centerNormal = GetSurfaceNormal(UV);
    float2 animatedNoisePixel = GetAnimatedNoisePixel(UV);

    float rawShadow = 1.0f;
    switch (ShadowAtlasData.UpscaleMode)
    {
    case SHADOW_UPSCALE_MODE_POINT:
        rawShadow = SampleShadowPoint(atlasPixelInt, meta);
        break;

    case SHADOW_UPSCALE_MODE_BILINEAR:
        rawShadow = SampleShadowBilinear(baseUV);
        break;

    case SHADOW_UPSCALE_MODE_PCF:
        rawShadow = ApplyKernelPCFFilter(
            meta,
            baseUV,
            minUV,
            maxUV,
            atlasSize,
            texelSize,
            (int)ShadowAtlasData.PCFKernelSize,
            false,
            centerDepth,
            centerNormal
        );
        break;

    case SHADOW_UPSCALE_MODE_JOINT_BILATERAL:
        rawShadow = ApplyJointBilateralRingFilter(
            meta,
            baseUV,
            minUV,
            maxUV,
            atlasSize,
            texelSize,
            centerDepth,
            centerNormal,
            animatedNoisePixel
        );
        break;

    case SHADOW_UPSCALE_MODE_JOINT_BILATERAL_PCF:
        rawShadow = ApplyKernelPCFFilter(
            meta,
            baseUV,
            minUV,
            maxUV,
            atlasSize,
            texelSize,
            (int)ShadowAtlasData.PCFKernelSize,
            true,
            centerDepth,
            centerNormal
        );
        break;

    case SHADOW_UPSCALE_MODE_POISSON_16:
        rawShadow = ApplyPoisson16Filter(
            meta,
            baseUV,
            minUV,
            maxUV,
            atlasSize,
            texelSize,
            false,
            centerDepth,
            centerNormal,
            animatedNoisePixel
        );
        break;

    case SHADOW_UPSCALE_MODE_JOINT_BILATERAL_POISSON_16:
        rawShadow = ApplyPoisson16Filter(
            meta,
            baseUV,
            minUV,
            maxUV,
            atlasSize,
            texelSize,
            true,
            centerDepth,
            centerNormal,
            animatedNoisePixel
        );
        break;

    default:
        rawShadow = ApplyJointBilateralRingFilter(
            meta,
            baseUV,
            minUV,
            maxUV,
            atlasSize,
            texelSize,
            centerDepth,
            centerNormal,
            animatedNoisePixel
        );
        break;
    }

    return ApplyBlackLevelAndContrast(rawShadow);
}


float3 ApplyPointLight(in LightStruct light, float2 UV, int id, float3 WorldPos, float3 N, float3 V, float3 albedo, float metallic, float roughness, float3 F0)
{
    float shadowFactor = GetShadowFactor(id, UV);
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
    float shadowFactor = GetShadowFactor(id, UV);
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
    float shadowFactor = GetShadowFactor(id, UV);
    if(shadowFactor<=0.0f) return float3(0.0f,0.0f,0.0f);

    float3 L = normalize(-light.Direction);
    float3 radiance = light.Color * light.Intensity;

    float3 resultLight = CalculatePBRLighting(N, V, L, radiance, albedo, metallic, roughness, F0);

    return resultLight * shadowFactor;
}


float3 ApplyRectLight(in LightStruct light, float2 UV, int id, float3 WorldPos, float3 N, float3 V, float3 albedo, float metallic, float roughness, float3 F0) {
    float shadowFactor = GetShadowFactor(id, UV);
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
    float4 nn = GBuffer_Normal.Sample(ss, uv).rgba;
    float selectionOutline = ComputeSelectionOutlineFactor(uv);
    if(nn.a > 2.5f) {
        psOut.result = float4(0,0,0,3.0f);
        return psOut;
    }

    float4 albedoOut = GBuffer_Albedo.Sample(ss, uv);
    float3 albedo = albedoOut.rgb;
    float alpha = albedoOut.a;

    float4 matData = GBuffer_MaterialData.Sample(ss, uv);
    float roughness = matData.r;
    float metallic = matData.g;
    float ao = matData.a;

    float4 emissiveTexture = GBuffer_Emissive.Sample(ss, uv);
    float3 emissive = emissiveTexture.rgb;
    float emissiveFactor = emissiveTexture.w;

    if(LHelper.IsUnlitScene != 0) {
        float3 color = albedo + emissive * emissiveFactor;
        color = lerp(color, SELECTION_OUTLINE_COLOR, selectionOutline);
        AlphaClipping(alpha);
        psOut.result = float4(color, alpha);
        return psOut;
    }

    float3 N = normalize(nn.rgb);
    float3 WorldPos = ReconstructWorldPosition(uv,  DepthBuffer.Sample(ss, uv).r, LHelper.InverseViewProjectionMatrix);
    float3 V = normalize(LHelper.CameraPos - WorldPos);

    float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);

    float3 posVS = mul(float4(WorldPos, 1.0), ClustersData.ViewMatrix).xyz;
    float  viewZAbs = abs(posVS.z);
    float2 fragCoord = float2(
        uv.x * (float)ClustersData.ScreenWidth,
        uv.y * (float)ClustersData.ScreenHeight
    );
    uint tileIndex = ComputeClusterIndex(fragCoord, viewZAbs, ClustersData);
    uint lightCount = Clusters[tileIndex].Count;
    
    float3 Lo = float3(0.0, 0.0, 0.0);
    for (int i = 0; i < lightCount; ++i) {
        
        uint lightIndex = Clusters[tileIndex].LightIndices[i];
        LightStruct light = Lights[lightIndex];

        switch (light.LightType) {
            case LIGHT_POINT_LIGHT_ENUM_VALUE:
                Lo += ApplyPointLight(light, uv, lightIndex, WorldPos, N, V, albedo, metallic, roughness, F0);
                break;
            case LIGHT_SPOT_LIGHT_ENUM_VALUE:
                Lo += ApplySpotLight(light, uv, lightIndex, WorldPos, N, V, albedo, metallic, roughness, F0);
                break;
            case LIGHT_DIRECTIONAL_LIGHT_ENUM_VALUE:
                Lo += ApplyDirectionalLight(light, uv, lightIndex, WorldPos, N, V, albedo, metallic, roughness, F0);
                break;
            case LIGHT_RECT_LIGHT_ENUM_VALUE:
                Lo += ApplyRectLight(light, uv, lightIndex, WorldPos, N, V, albedo, metallic, roughness, F0);
                break;
        }
    }
    
    float3 color = Lo;
    
    if (LHelper.IsIBLEnabled != 0) color += CalculateImageBasedLighting(N, V, albedo, metallic, roughness, ao, F0);

    color += emissive * emissiveFactor;
    color = lerp(color, SELECTION_OUTLINE_COLOR, selectionOutline);
    
    AlphaClipping(alpha);
    psOut.result = float4(color, alpha);
    return psOut;
}
