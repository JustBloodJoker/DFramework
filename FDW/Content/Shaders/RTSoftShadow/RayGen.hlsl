#include "Structures.hlsli"
#include "Utilits.hlsli"
#include "SameShadersStructs.hlsli"
#include "LTCFunctions.hlsli"

RaytracingAccelerationStructure Scene : register(t0);
RWTexture2D<float4> Output : register(u0);

Texture2D WorldPosGBuffer : register(t1);
Texture2D NormalGBuffer : register(t2);

ConstantBuffer<LightsHelper> LightsHelperBuffer : register(b1);
StructuredBuffer<LightStruct> Lights : register(t3);

float RandomFloat(inout uint seed)
{
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    return float(seed % 10000) / 10000.0f;
}

float3 SampleSphere(inout uint seed)
{
    float3 rand = float3(
        RandomFloat(seed),
        RandomFloat(seed),
        RandomFloat(seed)
    );

    float theta = 2.0f * 3.14159265f * rand.x;
    float phi = acos(1.0f - 2.0f * rand.y);
    float x = sin(phi) * cos(theta);
    float y = sin(phi) * sin(theta);
    float z = cos(phi);

    return float3(x, y, z);
}

float TraceShadowRay(float3 origin, float3 direction, float maxDist)
{
    RayDesc ray;
    ray.Origin = origin + direction * 0.01f;
    ray.Direction = direction;
    ray.TMin = 0.0f;
    ray.TMax = maxDist - 0.01f;

    ShadowPayload payload;
    payload.shadow = 1.0f;

    TraceRay(Scene, RAY_FLAG_NONE, 0xFF, 0, 1, 0, ray, payload);

    return payload.shadow;
}

float ComputeSoftShadow(float3 worldPos, float3 lightPos, float radius, int numSamples, inout uint seed)
{
    float shadowSum = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        float3 randDir = SampleSphere(seed);
        float3 samplePos = lightPos + randDir * radius;

        float3 toLight = samplePos - worldPos;
        float dist = length(toLight);
        float3 dir = toLight / dist;

        shadowSum += TraceShadowRay(worldPos, dir, dist);
    }

    return shadowSum / numSamples;
}

float ComputeRectLightShadow(float3 worldPos, float3 rectPoints[4], int numSamples, inout uint seed)
{
    float3 edgeX = rectPoints[1] - rectPoints[0];
    float3 edgeY = rectPoints[3] - rectPoints[0];

    float shadowSum = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        float2 uv = float2(RandomFloat(seed), RandomFloat(seed));
        float3 samplePos = rectPoints[0] + edgeX * uv.x + edgeY * uv.y;

        float3 toLight = samplePos - worldPos;
        float dist = length(toLight);
        float3 dir = toLight / dist;

        shadowSum += TraceShadowRay(worldPos, dir, dist);
    }

    return shadowSum / numSamples;
}


[shader("raygeneration")]
void RayGen()
{
    uint2 dispatchIndex = DispatchRaysIndex().xy;
    uint2 dispatchDim = DispatchRaysDimensions().xy;

    float3 worldPos = WorldPosGBuffer.Load(int3(dispatchIndex, 0)).xyz;

    float totalShadow = 0.0f;
    int totalLights = 0;

    for (int i = 0; i < LightsHelperBuffer.LightCount; ++i)
    {
        LightStruct light = Lights[i];

        uint seed = dispatchIndex.x * 1973 + dispatchIndex.y * 9277 + i * 26699;
        float shadow = 1.0f;

        if (light.LightType == LIGHT_DIRECTIONAL_LIGHT_ENUM_VALUE)
        {
            float3 dir = -normalize(light.Direction);
            shadow = TraceShadowRay(worldPos, dir, 100000.0f);
        }
        else if (light.LightType == LIGHT_POINT_LIGHT_ENUM_VALUE)
        {
            shadow = ComputeSoftShadow(worldPos, light.Position, light.SourceRadius, 16, seed);
        }
        else if (light.LightType == LIGHT_SPOT_LIGHT_ENUM_VALUE)
        {
            float3 toLight = light.Position - worldPos;
            float dist = length(toLight);
            float3 dir = normalize(toLight);
            float3 spotDir = normalize(-light.Direction);

            float cosAngle = dot(dir, spotDir);
            float cosOuter = cos(light.OuterConeAngle);
            float cosInner = cos(light.InnerConeAngle);

            if (cosAngle >= cosOuter)
            {
                shadow = TraceShadowRay(worldPos, dir, dist);
            }
            else
            {
                continue;
            }
        }
        else if (light.LightType == LIGHT_RECT_LIGHT_ENUM_VALUE)
        {
            float3 rectPoints[4];
            InitRectPoints(light, rectPoints);
            shadow = ComputeRectLightShadow(worldPos, rectPoints, 16, seed);
        }

        totalShadow += shadow;
        totalLights++;
    }
    float finalShadow = (totalLights > 0) ? (totalShadow / totalLights) : 1.0f;
    Output[dispatchIndex] = float4(finalShadow, finalShadow, finalShadow, 1.0f);
}