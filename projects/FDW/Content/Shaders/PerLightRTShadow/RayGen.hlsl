#include "SameShadersStructs.hlsli"
#include "Structures.hlsli"
#include "Utilits.hlsli"
#include "NeuralShadowRayTracingData.hlsli"

ConstantBuffer<PerLightRTReferenceParams> ReferenceData : register(b0);
#define AtlasData ReferenceData.Atlas
RaytracingAccelerationStructure SceneBVH : register(t0);
StructuredBuffer<LightStruct> Lights : register(t1);
StructuredBuffer<LightAtlasMeta> LightAtlasMetas : register(t2);
Texture2D<uint> TexelToLight : register(t3);
Texture2D<float4> DepthBuffer : register(t4);
Texture2D<float4> GBufferNormal : register(t5);
SamplerState samplerLinearClamp : register(s0);

RWTexture2D<float> PerLightVisibility : register(u0);

#define RT_SHADOW_QUALITY ReferenceData.Quality
#define RT_SHADOW_OUTPUT_WIDTH AtlasData.ScreenWidth
#define RT_SHADOW_OUTPUT_HEIGHT AtlasData.ScreenHeight
#define RT_SHADOW_INVERSE_VIEW_PROJECTION AtlasData.InverseViewProjectionMatrix
#include "../ShadowCommon/RTShadowSampling.hlsli"

[shader("raygeneration")]
void RayGen()
{
    uint2 atlasPixel = DispatchRaysIndex().xy;
    if (atlasPixel.x >= AtlasData.AtlasWidth || atlasPixel.y >= AtlasData.AtlasHeight)
        return;

    uint metaIndex = TexelToLight.Load(int3(atlasPixel, 0));
    if (metaIndex >= 0xFFu)
    {
        PerLightVisibility[atlasPixel] = 1.0f;
        return;
    }

    LightAtlasMeta meta = LightAtlasMetas[metaIndex];
    // padd1 is runtime-only selection state. Unselected atlas tiles retain the
    // hard-shadow value produced earlier in the frame.
    if (meta.padd1 == 0u)
        return;
    LightStruct light = Lights[meta.LightIndex];

    uint2 tilePixel = atlasPixel - uint2(meta.AtlasOffsetX, meta.AtlasOffsetY);
    float2 tileUV = (float2(tilePixel) + 0.5f) / float2(meta.AtlasWidth, meta.AtlasHeight);
    float2 screenUV = float2(
        lerp(meta.ScreenMinU, meta.ScreenMaxU, tileUV.x),
        lerp(meta.ScreenMinV, meta.ScreenMaxV, tileUV.y));
    uint2 gBufferPixel = min(
        uint2(floor(screenUV * float2(AtlasData.ScreenWidth, AtlasData.ScreenHeight))),
        uint2(AtlasData.ScreenWidth - 1, AtlasData.ScreenHeight - 1));

    float3 worldPosition = WorldPosition(gBufferPixel);
    float3 shadingNormal;
    if (!TryNormalize(GBufferNormal.Load(int3(gBufferPixel, 0)).xyz, shadingNormal))
    {
        PerLightVisibility[atlasPixel] = 1.0f;
        return;
    }
    float pixelWorldSize = 0.0f;
    float3 receiverNormal = ReceiverNormal(gBufferPixel, shadingNormal, pixelWorldSize);
    float visibility = 1.0f;
    float shadowSoftness = RTShadowEffectiveSoftness(light);
    float effectiveSourceRadius = light.SourceRadius * shadowSoftness;
    float effectiveAngularRadius = light.DirectionalAngularRadius * shadowSoftness;
    float2 effectiveRectSize = light.RectSize * shadowSoftness;

    if (light.LightType == LIGHT_DIRECTIONAL_LIGHT_ENUM_VALUE &&
        effectiveAngularRadius > 0.0f && ReferenceData.Quality.IsOffline != 0u)
    {
        visibility = AccumulateFiniteDirectionalVisibility(
            light, meta.LightIndex, gBufferPixel, worldPosition, receiverNormal, pixelWorldSize);
    }
    else if (light.LightType == LIGHT_DIRECTIONAL_LIGHT_ENUM_VALUE && effectiveAngularRadius <= 0.0f)
    {
        float3 lightDirection;
        if (TryNormalize(-light.Direction, lightDirection))
        {
            visibility = TraceShadowRay(
                worldPosition,
                receiverNormal,
                lightDirection,
                ReferenceData.Quality.DirectionalTMax,
                pixelWorldSize);
        }
    }
    else if (light.LightType == LIGHT_POINT_LIGHT_ENUM_VALUE && effectiveSourceRadius <= 0.0f)
    {
        float3 toLight = light.Position - worldPosition;
        float distanceToLight = length(toLight);
        if (distanceToLight > 0.0f && (light.AttenuationRadius <= 0.0f || distanceToLight <= light.AttenuationRadius))
        {
            visibility = TraceShadowRay(
                worldPosition,
                receiverNormal,
                toLight / distanceToLight,
                distanceToLight,
                pixelWorldSize);
        }
    }
    else if (light.LightType == LIGHT_POINT_LIGHT_ENUM_VALUE &&
        effectiveSourceRadius > 0.0f && ReferenceData.Quality.IsOffline != 0u)
    {
        float distanceToCenter = length(light.Position - worldPosition);
        if (distanceToCenter > 0.0f &&
            (light.AttenuationRadius <= 0.0f || distanceToCenter <= light.AttenuationRadius))
        {
            visibility = AccumulateFinitePointVisibility(
                light,
                meta.LightIndex,
                gBufferPixel,
                worldPosition,
                receiverNormal,
                pixelWorldSize);
        }
    }
    else if (light.LightType == LIGHT_SPOT_LIGHT_ENUM_VALUE &&
        effectiveSourceRadius > 0.0f && ReferenceData.Quality.IsOffline != 0u)
    {
        float distanceToCenter = length(light.Position - worldPosition);
        if (distanceToCenter > 0.0f &&
            (light.AttenuationRadius <= 0.0f || distanceToCenter <= light.AttenuationRadius))
        {
            visibility = AccumulateFiniteSpotVisibility(
                light, meta.LightIndex, gBufferPixel, worldPosition, receiverNormal, pixelWorldSize);
        }
    }
    else if (light.LightType == LIGHT_SPOT_LIGHT_ENUM_VALUE && effectiveSourceRadius <= 0.0f)
    {
        float3 toLight = light.Position - worldPosition;
        float distanceToLight = length(toLight);
        if (distanceToLight > 0.0f && (light.AttenuationRadius <= 0.0f || distanceToLight <= light.AttenuationRadius))
        {
            float3 fromLight = -toLight / distanceToLight;
            float cosOuter = cos(light.OuterConeAngle);
            float3 spotDirection;
            if (TryNormalize(light.Direction, spotDirection) && dot(spotDirection, fromLight) >= cosOuter)
            {
                visibility = TraceShadowRay(
                    worldPosition,
                    receiverNormal,
                    toLight / distanceToLight,
                    distanceToLight,
                    pixelWorldSize);
            }
        }
    }
    else if (light.LightType == LIGHT_RECT_LIGHT_ENUM_VALUE &&
        effectiveRectSize.x > 0.0f && effectiveRectSize.y > 0.0f && ReferenceData.Quality.IsOffline != 0u)
    {
        visibility = AccumulateRectangleVisibility(
            light,
            meta.LightIndex,
            gBufferPixel,
            worldPosition,
            receiverNormal,
            pixelWorldSize);
    }
    else if (light.LightType == LIGHT_RECT_LIGHT_ENUM_VALUE &&
        (effectiveRectSize.x <= 0.0f || effectiveRectSize.y <= 0.0f))
    {
        float3 toLight = light.Position - worldPosition;
        float distanceToLight = length(toLight);
        if (distanceToLight > 0.0f)
            visibility = TraceShadowRay(
                worldPosition, receiverNormal, toLight / distanceToLight, distanceToLight, pixelWorldSize);
    }

    PerLightVisibility[atlasPixel] = visibility;
}
