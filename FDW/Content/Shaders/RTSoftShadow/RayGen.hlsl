#include "Structures.hlsli"
#include "Utilits.hlsli"
#include "SameShadersStructs.hlsli"
#include "LTCFunctions.hlsli"
#include "ClusteredLightsUtils.hlsli"

RaytracingAccelerationStructure Scene : register(t0);
RWTexture2D<float4> Output : register(u0);

Texture2D WorldPosGBuffer : register(t1);
Texture2D NormalGBuffer   : register(t2);

ConstantBuffer<LightsHelper> LightsHelperBuffer : register(b1);
StructuredBuffer<LightStruct> Lights : register(t3);

ConstantBuffer<RTSoftShadowFrameStruct> SoftShadowFrameData : register(b2);
Texture2D PrevWorldPosAndShadowFactorBuffer : register(t4);
RWTexture2D<float4> CurrentWorldPosAndShadowFactor : register(u1);

SamplerState LinearClampSampler : register(s0);


StructuredBuffer<Cluster> Clusters : register(t5);
ConstantBuffer<ClusterParamsPS> ClustersData : register(b3);

float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

float2 Hammersley(uint i, uint N)
{
    return float2(float(i) / float(N), RadicalInverse_VdC(i));
}

float3 HemisphereSample(float2 u)
{
    float phi = 2.0f * 3.14159265f * u.x;
    float cosTheta = 1.0f - u.y;
    float sinTheta = sqrt(1.0f - cosTheta * cosTheta);
    return float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}

float TraceShadowRay(float3 origin, float3 direction, float maxDist)
{
    RayDesc ray;
    ray.Origin = origin + direction * 0.5f;
    ray.Direction = direction;
    ray.TMin = 0.0f;
    ray.TMax = maxDist - 0.01f;

    ShadowPayload payload;
    payload.shadow = 1.0f;

    TraceRay(Scene, RAY_FLAG_NONE, 0xFF, 0, 1, 0, ray, payload);

    return payload.shadow;
}

float ComputeSoftShadow_Point(float3 worldPos, float3 lightPos, float radius, float attRad, uint pixelSeed)
{
    const uint NumSamples = 4;
    float shadow = 0;

    for (uint i = 0; i < NumSamples; i++)
    {
        float2 xi = Hammersley(i + pixelSeed, NumSamples + pixelSeed);
        float3 randDir = HemisphereSample(xi);

        float3 samplePos = lightPos + randDir * radius;
        float3 toLight = samplePos - worldPos;

        float dist = length(toLight);
        float3 dir = toLight / dist;

        shadow += TraceShadowRay(worldPos, dir, dist);
    }

    return shadow / NumSamples;
}

float3 RectLightNormal(float3 pts[4])
{
    return normalize(cross(pts[1] - pts[0], pts[3] - pts[0]));
}

float ComputeSoftShadow_Rect(float3 worldPos, float3 rectPoints[4], uint pixelSeed, float3 lightNormal)
{
    const uint NumSamples = 4;
    float acc = 0.0f;
    uint  cnt = 0u;

    float3 edgeX = rectPoints[1] - rectPoints[0];
    float3 edgeY = rectPoints[3] - rectPoints[0];

    [unroll]
    for (uint i = 0; i < NumSamples; i++)
    {
        float2 xi = Hammersley(i + pixelSeed, NumSamples + pixelSeed);
        float3 samplePos = rectPoints[0] + edgeX * xi.x + edgeY * xi.y;

        float3 toLight = samplePos - worldPos;
        float  dist = length(toLight);
        float3 dir = toLight / max(dist, 1e-4);

       if (dot(dir, lightNormal) <= 0.0f)
            continue;

        acc += TraceShadowRay(worldPos, dir, dist);
        cnt++;
    }

   return (cnt > 0u) ? (acc / cnt) : 1.0f;
}

[shader("raygeneration")]
void RayGen()
{
    uint2 dispatchIndex = DispatchRaysIndex().xy;

    float3 worldPos = WorldPosGBuffer.Load(int3(dispatchIndex, 0)).xyz;
    float3 normal   = NormalGBuffer.Load(int3(dispatchIndex, 0)).xyz;

    float3 posVS = mul(float4(worldPos, 1.0), ClustersData.ViewMatrix).xyz;
    float  viewZAbs = abs(posVS.z);

    float2 fragCoord = float2(dispatchIndex) + 0.5f;

    uint tileIndex = ComputeClusterIndex(fragCoord, viewZAbs, ClustersData);
    uint lightCount = Clusters[tileIndex].Count;

    float totalShadow = 0.0f;
    int   totalLights = 0;

    for (int i = 0; i < lightCount; ++i)
    { 
        uint lightIndex = Clusters[tileIndex].LightIndices[i];
        LightStruct light = Lights[lightIndex];
        uint pixelSeed = dispatchIndex.x * 1973 + dispatchIndex.y * 9277 + i * 26699 + SoftShadowFrameData.FrameIndex * 17;

        float shadow = 1.0f;

        if (light.LightType == LIGHT_DIRECTIONAL_LIGHT_ENUM_VALUE)
        {
            float3 dir = -normalize(light.Direction);
            shadow = TraceShadowRay(worldPos, dir, 100000.0f);
        }
        else if (light.LightType == LIGHT_POINT_LIGHT_ENUM_VALUE)
        {
            if(length(light.Position - worldPos) > light.AttenuationRadius) continue;
            
            shadow = ComputeSoftShadow_Point(worldPos, light.Position, light.SourceRadius, light.AttenuationRadius, pixelSeed);
        }
        else if (light.LightType == LIGHT_SPOT_LIGHT_ENUM_VALUE)
        {
            float3 toLight = light.Position - worldPos;
            float  dist = length(toLight);
            float3 dir = normalize(toLight);
            float3 spotDir = normalize(-light.Direction);

            float cosAngle = dot(dir, spotDir);
            if(light.AttenuationRadius<dist) continue;

            shadow = TraceShadowRay(worldPos, dir, dist);
        }
        else if (light.LightType == LIGHT_RECT_LIGHT_ENUM_VALUE)
        {
            float3 rectPoints[4];
            InitRectPoints(light, rectPoints);
            float3 lightN = RectLightNormal(rectPoints);
            float3 toCenter = light.Position - worldPos;
            if (dot(toCenter, lightN) <= 0.0f)
                continue;

            shadow = ComputeSoftShadow_Rect(worldPos, rectPoints, pixelSeed, lightN);
        }

        totalShadow += shadow;
        totalLights++;
    }

    float currShadow = (totalLights > 0) ? (totalShadow / totalLights) : 1.0f;

    float4 prevClip = mul(float4(worldPos, 1.0f), SoftShadowFrameData.PrevViewProj);
    float2 prevUV   = prevClip.xy / prevClip.w * 0.5f + 0.5f;
    bool   validPrev = all(prevUV >= 0.0f) && all(prevUV <= 1.0f);

    float finalShadow = currShadow;
    if (validPrev)
    {
        float4 prevData = PrevWorldPosAndShadowFactorBuffer.SampleLevel(LinearClampSampler, prevUV, 0);
        float3 prevPos  = prevData.xyz;
        float  prevFac  = prevData.w;

        float distDiff = distance(prevPos, worldPos);
        float normDiff = abs(dot(normal, normalize(prevPos - worldPos)));

        if (distDiff < SoftShadowFrameData.ReprojDistThreshold && normDiff < SoftShadowFrameData.NormalThreshold)
        {
            float feedback = lerp(SoftShadowFrameData.TemporalFeedbackMin,
                                  SoftShadowFrameData.TemporalFeedbackMax,
                                  saturate(1.0f - distDiff / SoftShadowFrameData.ReprojDistThreshold));
            finalShadow = lerp(currShadow, prevFac, feedback);
        }
    }

    Output[dispatchIndex]                         = float4(finalShadow, finalShadow, finalShadow, 1.0f);
    CurrentWorldPosAndShadowFactor[dispatchIndex] = float4(worldPos, finalShadow);
}
