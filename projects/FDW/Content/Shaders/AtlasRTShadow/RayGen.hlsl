#include "SameShadersStructs.hlsli"
#include "Structures.hlsli"
#include "Utilits.hlsli"

ConstantBuffer<ShadowAtlasParams> AtlasData : register(b0);
RaytracingAccelerationStructure SceneBVH : register(t0);
StructuredBuffer<LightStruct> Lights : register(t1);
StructuredBuffer<LightAtlasMeta> LightAtlasMetas : register(t2);
Texture2D<uint> TexelToLight : register(t3);
Texture2D<float4> DepthBuffer : register(t4);
Texture2D<float4> GBufferNormal : register(t5);
SamplerState samplerLinearClamp : register(s0);

RWTexture2D<float> ShadowAtlasUAV : register(u0);
RWTexture2D<float> BlockerDistanceAtlasUAV : register(u1);
RWTexture2D<uint> BlockerValidityAtlasUAV : register(u2);

static const uint NS_BLOCKER_HARD_VALID = 1u;
static const uint NS_BLOCKER_DISTANCE_VALID = 2u;

float3 ReconstructWorldPositionFromPixel(uint2 gBufferPixel)
{
    float2 uv = (float2(gBufferPixel) + 0.5f) / float2(AtlasData.ScreenWidth, AtlasData.ScreenHeight);
    float depth = DepthBuffer.Load(int3(gBufferPixel, 0)).x;
    return ReconstructWorldPosition(uv, depth, AtlasData.InverseViewProjectionMatrix);
}

float3 ComputeDepthNormal(uint2 gBufferPixel, float3 fallbackNormal, out float pixelWorldSize)
{
    uint2 maxPixel = uint2(AtlasData.ScreenWidth - 1, AtlasData.ScreenHeight - 1);
    uint2 leftPixel = uint2(gBufferPixel.x > 0 ? gBufferPixel.x - 1 : gBufferPixel.x, gBufferPixel.y);
    uint2 rightPixel = uint2(min(gBufferPixel.x + 1, maxPixel.x), gBufferPixel.y);
    uint2 upPixel = uint2(gBufferPixel.x, gBufferPixel.y > 0 ? gBufferPixel.y - 1 : gBufferPixel.y);
    uint2 downPixel = uint2(gBufferPixel.x, min(gBufferPixel.y + 1, maxPixel.y));

    float3 centerPos = ReconstructWorldPositionFromPixel(gBufferPixel);
    float3 leftPos = ReconstructWorldPositionFromPixel(leftPixel);
    float3 rightPos = ReconstructWorldPositionFromPixel(rightPixel);
    float3 upPos = ReconstructWorldPositionFromPixel(upPixel);
    float3 downPos = ReconstructWorldPositionFromPixel(downPixel);

    float3 dxForward = rightPos - centerPos;
    float3 dxBackward = centerPos - leftPos;
    float3 dyForward = downPos - centerPos;
    float3 dyBackward = centerPos - upPos;

    float3 dx = dot(dxForward, dxForward) < dot(dxBackward, dxBackward) ? dxForward : dxBackward;
    float3 dy = dot(dyForward, dyForward) < dot(dyBackward, dyBackward) ? dyForward : dyBackward;
    pixelWorldSize = max(length(dx), length(dy));

    float3 depthNormal = cross(dy, dx);
    float normalLenSq = dot(depthNormal, depthNormal);
    if (normalLenSq <= 1e-10f) {
        return fallbackNormal;
    }

    depthNormal *= rsqrt(normalLenSq);
    return dot(depthNormal, fallbackNormal) < 0.0f ? -depthNormal : depthNormal;
}

bool TraceShadowRay(float3 origin, float3 normal, float3 dir, float dist, float pixelWorldSize, out float2 result)
{
    RayDesc ray;
    
    float NdotL = dot(normal, dir);
    float baseBias = 0.0002f; 
    float bias = clamp(baseBias * sqrt(1.0f - NdotL * NdotL) / max(NdotL, 1e-5f), 0.003f, 0.07f);
    bias = clamp(max(bias, pixelWorldSize * 0.65f), 0.03f, 1.75f);

    float rayEpsilon = max(0.03f, bias * 0.7f);
    ray.Origin = origin + normal * bias + dir * rayEpsilon; 
    
    ray.Direction = dir;
    ray.TMin = rayEpsilon;
    ray.TMax = dist - bias - rayEpsilon;
    if (!all(isfinite(float4(ray.Origin, ray.TMax))) || !all(isfinite(ray.Direction)) ||
        !isfinite(ray.TMin) || ray.TMax <= ray.TMin)
    {
        result = float2(1.0f, 1.0f);
        return false;
    }

    ShadowPayload payload;
    payload.Visible = 1.0f;
    payload.BlockerDistance = dist;
    
    TraceRay(SceneBVH, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, 0xFF, 0, 1, 0, ray, payload);

    result = float2(
        payload.Visible,
        payload.Visible >= 0.5f ? 1.0f : saturate(payload.BlockerDistance / max(dist, 1e-6f)));
    return all(isfinite(result));
}

[shader("raygeneration")]
void RayGen()
{
    uint2 pixel = DispatchRaysIndex().xy;
    if (pixel.x >= AtlasData.AtlasWidth || pixel.y >= AtlasData.AtlasHeight) 
        return;

    uint texelLookup = TexelToLight.Load(int3(pixel,0));
    if (texelLookup >= 0xFFu) {
        ShadowAtlasUAV[pixel] = 1.0f;
        BlockerDistanceAtlasUAV[pixel] = 1.0f;
        BlockerValidityAtlasUAV[pixel] = 0u;
        return;
    }

    LightAtlasMeta meta = LightAtlasMetas[texelLookup];
    LightStruct light   = Lights[meta.LightIndex];

    uint2 localPixel = uint2(meta.AtlasOffsetX, meta.AtlasOffsetY);
    uint2 dtPixel = pixel - localPixel;
    float2 uvLocal = (float2(dtPixel) + 0.5f) / float2(meta.AtlasWidth, meta.AtlasHeight);

    float screenU = lerp(meta.ScreenMinU, meta.ScreenMaxU, uvLocal.x);
    float screenV = lerp(meta.ScreenMinV, meta.ScreenMaxV, uvLocal.y);
    
    float2 screenPos = float2(screenU * AtlasData.ScreenWidth, screenV * AtlasData.ScreenHeight);

    uint2 gBuffersPixel = uint2(floor(screenPos)); 
    
    gBuffersPixel.x = min(gBuffersPixel.x, AtlasData.ScreenWidth - 1);
    gBuffersPixel.y = min(gBuffersPixel.y, AtlasData.ScreenHeight - 1);
    
    float visible = 1.0f;
    float blockerDistance = 1.0f;
    uint validity = 0u;

    float receiverDepth = DepthBuffer.Load(int3(gBuffersPixel, 0)).x;
    if (!isfinite(receiverDepth) || receiverDepth < 0.0f || receiverDepth >= 1.0f)
    {
        ShadowAtlasUAV[pixel] = visible;
        BlockerDistanceAtlasUAV[pixel] = blockerDistance;
        BlockerValidityAtlasUAV[pixel] = 0u;
        return;
    }

    float3 worldPos = ReconstructWorldPositionFromPixel(gBuffersPixel);
    float3 shadingNormal = GBufferNormal.Load(int3(gBuffersPixel,0)).xyz;
    float shadingNormalLengthSquared = dot(shadingNormal, shadingNormal);
    if (!all(isfinite(worldPos)))
    {
        ShadowAtlasUAV[pixel] = visible;
        BlockerDistanceAtlasUAV[pixel] = blockerDistance;
        BlockerValidityAtlasUAV[pixel] = 0u;
        return;
    }
    if (!isfinite(shadingNormalLengthSquared) || shadingNormalLengthSquared <= 1e-10f)
    {
        ShadowAtlasUAV[pixel] = visible;
        BlockerDistanceAtlasUAV[pixel] = blockerDistance;
        BlockerValidityAtlasUAV[pixel] = 0u;
        return;
    }
    shadingNormal *= rsqrt(shadingNormalLengthSquared);
    float pixelWorldSize = 0.0f;
    float3 normal = ComputeDepthNormal(gBuffersPixel, shadingNormal, pixelWorldSize);

    if (light.LightType == LIGHT_DIRECTIONAL_LIGHT_ENUM_VALUE)
    {
        float3 lightDir = -normalize(light.Direction);
        const float INF_DIST = 1e6f;
        float2 traceResult;
        validity = NS_BLOCKER_HARD_VALID;
        if (TraceShadowRay(worldPos, normal, lightDir, INF_DIST, pixelWorldSize, traceResult))
        {
            visible = traceResult.x;
            blockerDistance = traceResult.y;
            validity |= NS_BLOCKER_DISTANCE_VALID;
        }
    }
    else if (light.LightType == LIGHT_POINT_LIGHT_ENUM_VALUE)
    {
        float3 toLight = light.Position - worldPos;
        float dist = length(toLight);
        if (dist <= 0.0f) {
            visible = 0.0f;
        } else {
            validity = NS_BLOCKER_HARD_VALID;
            if (light.AttenuationRadius > 0.0f && dist > light.AttenuationRadius)
            {
                visible = 0.0f;
            }
            else
            {
                float3 dir = toLight / dist;
                float2 traceResult;
                if (TraceShadowRay(worldPos, normal, dir, dist, pixelWorldSize, traceResult))
                {
                    visible = traceResult.x;
                    blockerDistance = traceResult.y;
                    validity |= NS_BLOCKER_DISTANCE_VALID;
                }
            }
        }
    }
    else if (light.LightType == LIGHT_SPOT_LIGHT_ENUM_VALUE)
    {
        float3 toLight = light.Position - worldPos;
        float dist = length(toLight);
        if (dist <= 0.0f) {
            visible = 0.0f;
        } else
        {
            validity = NS_BLOCKER_HARD_VALID;
            float3 L = normalize(worldPos - light.Position);
            float3 spotDir = normalize(light.Direction);
            float cosAngle = dot(spotDir, L);
            float cosOuter = cos(light.OuterConeAngle);
            if ( (light.AttenuationRadius > 0.0f && dist > light.AttenuationRadius) || cosAngle < cosOuter)
            {
                visible = 0.0f;
            } else 
            {
                float3 dir = normalize(toLight);
                float2 traceResult;
                if (TraceShadowRay(worldPos, normal, dir, dist, pixelWorldSize, traceResult))
                {
                    visible = traceResult.x;
                    blockerDistance = traceResult.y;
                    validity |= NS_BLOCKER_DISTANCE_VALID;
                }
            }
            
        }
    }
    else
    {
        //todo Rect light shadow
        visible = 1.0f;
    }

    ShadowAtlasUAV[pixel] = visible;
    BlockerDistanceAtlasUAV[pixel] = blockerDistance;
    BlockerValidityAtlasUAV[pixel] = validity;
}
