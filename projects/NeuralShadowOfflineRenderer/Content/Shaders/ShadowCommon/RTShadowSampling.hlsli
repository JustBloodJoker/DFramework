#ifndef FDW_RT_SHADOW_SAMPLING_HLSLI
#define FDW_RT_SHADOW_SAMPLING_HLSLI

#include "RTShadowBias.hlsli"

bool TryNormalize(float3 value, out float3 normalizedValue)
{
    float lengthSq = dot(value, value);
    normalizedValue = lengthSq > 1e-10f ? value * rsqrt(lengthSq) : 0.0f;
    return lengthSq > 1e-10f && all(isfinite(value));
}

uint PcgHash(uint input)
{
    uint state = input * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

uint RTShadowSeedHash(uint lightIndex, uint2 pixel, uint sampleIndex)
{
    uint hash = RT_SHADOW_QUALITY.BaseSeed;
    hash = PcgHash(hash ^ RT_SHADOW_QUALITY.QualityMode);
    hash = PcgHash(hash ^ RT_SHADOW_QUALITY.GenerationLow);
    hash = PcgHash(hash ^ RT_SHADOW_QUALITY.GenerationHigh);
    hash = PcgHash(hash ^ lightIndex);
    hash = PcgHash(hash ^ pixel.x);
    hash = PcgHash(hash ^ pixel.y);
    return PcgHash(hash ^ sampleIndex);
}

float Random(uint lightIndex, uint2 pixel, uint sampleIndex, uint dimension)
{
    uint seed = RTShadowSeedHash(lightIndex, pixel, sampleIndex);
    uint value = PcgHash(seed ^ (0x9e3779b9u * (dimension + 1u)));
    return (float(value) + 0.5f) * (1.0f / 4294967296.0f);
}

void BuildBasis(float3 direction, out float3 tangent, out float3 bitangent)
{
    float3 helper = abs(direction.z) < 0.999f ? float3(0, 0, 1) : float3(0, 1, 0);
    tangent = normalize(cross(helper, direction));
    bitangent = cross(direction, tangent);
}

float RTShadowEffectiveSoftness(LightStruct light)
{
    return isfinite(light.ShadowSoftness) ? clamp(light.ShadowSoftness, 0.0f, 4.0f) : 1.0f;
}

float3 WorldPosition(uint2 pixel)
{
    float2 uv = (float2(pixel) + 0.5f) /
        float2(RT_SHADOW_OUTPUT_WIDTH, RT_SHADOW_OUTPUT_HEIGHT);
    return ReconstructWorldPosition(
        uv, DepthBuffer.Load(int3(pixel, 0)).r, RT_SHADOW_INVERSE_VIEW_PROJECTION);
}

float3 ReceiverNormal(uint2 pixel, float3 fallback, out float pixelWorldSize)
{
    uint2 limit = uint2(RT_SHADOW_OUTPUT_WIDTH - 1u, RT_SHADOW_OUTPUT_HEIGHT - 1u);
    uint2 l = uint2(pixel.x > 0u ? pixel.x - 1u : pixel.x, pixel.y);
    uint2 r = uint2(min(pixel.x + 1u, limit.x), pixel.y);
    uint2 u = uint2(pixel.x, pixel.y > 0u ? pixel.y - 1u : pixel.y);
    uint2 d = uint2(pixel.x, min(pixel.y + 1u, limit.y));
    float3 center = WorldPosition(pixel);
    float3 dx0 = WorldPosition(r) - center;
    float3 dx1 = center - WorldPosition(l);
    float3 dy0 = WorldPosition(d) - center;
    float3 dy1 = center - WorldPosition(u);
    float3 dx = dot(dx0, dx0) < dot(dx1, dx1) ? dx0 : dx1;
    float3 dy = dot(dy0, dy0) < dot(dy1, dy1) ? dy0 : dy1;
    pixelWorldSize = max(length(dx), length(dy));
    float3 geometric;
    if (!TryNormalize(cross(dy, dx), geometric)) {
        return fallback;
    }
    return dot(geometric, fallback) < 0.0f ? -geometric : geometric;
}

bool TraceVisibility(float3 origin, float3 normal, float3 direction,
    float distanceToEmitter, float pixelWorldSize, out float visibility)
{
    visibility = 1.0f;
    float normalBias;
    float epsilon;
    RTShadowComputeBias(normal, direction, pixelWorldSize, normalBias, epsilon);
    float tMax = distanceToEmitter - normalBias - epsilon;
    if (!all(isfinite(float4(origin, tMax))) ||
        !all(isfinite(float4(direction, pixelWorldSize))) || tMax <= epsilon) {
        return false;
    }
    RayDesc ray;
    ray.Origin = origin + normal * normalBias + direction * epsilon;
    ray.Direction = direction;
    ray.TMin = epsilon;
    ray.TMax = tMax;
    ShadowPayload payload;
    payload.Visible = 1.0f;
    TraceRay(SceneBVH, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, 0xff, 0, 1, 0, ray, payload);
    visibility = payload.Visible;
    return true;
}

float TraceShadowRay(float3 origin, float3 normal, float3 direction,
    float distanceToEmitter, float pixelWorldSize)
{
    float visibility;
    return TraceVisibility(origin, normal, direction, distanceToEmitter, pixelWorldSize, visibility)
        ? visibility : 1.0f;
}

bool PointOrSpotSample(LightStruct light, uint lightIndex, uint2 pixel, float3 position,
    float3 normal, float pixelWorldSize, uint sampleIndex, bool spot,
    bool spotBasisUsesEmitterDirection, out float visibility)
{
    visibility = 1.0f;
    float radius = light.SourceRadius * RTShadowEffectiveSoftness(light);
    float3 toCenter = light.Position - position;
    float centerDistance = length(toCenter);
    if (centerDistance <= 1e-6f ||
        (light.AttenuationRadius > 0.0f && centerDistance > light.AttenuationRadius)) {
        return false;
    }
    float3 spotDirection;
    if (spot && (!TryNormalize(light.Direction, spotDirection) ||
        dot(spotDirection, -toCenter / centerDistance) < cos(light.OuterConeAngle))) {
        return false;
    }
    float3 samplePosition = light.Position;
    if (radius > 0.0f) {
        float3 basisDirection = spot && spotBasisUsesEmitterDirection
            ? spotDirection : toCenter / centerDistance;
        float3 tangent, bitangent;
        BuildBasis(basisDirection, tangent, bitangent);
        float diskRadius = sqrt(Random(lightIndex, pixel, sampleIndex, 0u));
        float angle = 6.28318530718f * Random(lightIndex, pixel, sampleIndex, 1u);
        samplePosition += radius * diskRadius * (tangent * cos(angle) + bitangent * sin(angle));
    }
    float3 toSample = samplePosition - position;
    float distanceToSample = length(toSample);
    return distanceToSample > 1e-6f && TraceVisibility(position, normal,
        toSample / distanceToSample, distanceToSample, pixelWorldSize, visibility);
}

bool DirectionalSample(LightStruct light, uint lightIndex, uint2 pixel, float3 position,
    float3 normal, float pixelWorldSize, uint sampleIndex, out float visibility)
{
    visibility = 1.0f;
    float3 direction;
    if (!TryNormalize(-light.Direction, direction)) {
        return false;
    }
    float angularRadius = min(light.DirectionalAngularRadius *
        RTShadowEffectiveSoftness(light), 0.5f);
    if (angularRadius > 0.0f) {
        float3 tangent, bitangent;
        BuildBasis(direction, tangent, bitangent);
        float cosTheta = lerp(1.0f, cos(angularRadius), Random(lightIndex, pixel, sampleIndex, 0u));
        float sinTheta = sqrt(max(0.0f, 1.0f - cosTheta * cosTheta));
        float phi = 6.28318530718f * Random(lightIndex, pixel, sampleIndex, 1u);
        direction = direction * cosTheta + sinTheta *
            (tangent * cos(phi) + bitangent * sin(phi));
    }
    return TraceVisibility(position, normal, direction,
        RT_SHADOW_QUALITY.DirectionalTMax, pixelWorldSize, visibility);
}

bool RectangleSample(LightStruct light, uint lightIndex, uint2 pixel, float3 position,
    float3 normal, float pixelWorldSize, uint sampleIndex, bool requireFrontFace,
    out float visibility)
{
    visibility = 1.0f;
    float2 size = light.RectSize * RTShadowEffectiveSoftness(light);
    if (!all(isfinite(size)) || size.x <= 0.0f || size.y <= 0.0f) {
        return false;
    }
    float3 axisX = RotateAxisQuat(float3(1, 0, 0), light.Rotation.x, light.Rotation.y, light.Rotation.z);
    float3 axisY = RotateAxisQuat(float3(0, 1, 0), light.Rotation.x, light.Rotation.y, light.Rotation.z);
    float3 emitterNormal;
    if (!TryNormalize(cross(axisX, axisY), emitterNormal)) {
        return false;
    }
    float3 toCenter = light.Position - position;
    float centerDistance = length(toCenter);
    if (centerDistance <= 1e-6f ||
        (requireFrontFace && dot(toCenter / centerDistance, emitterNormal) <= 0.0f)) {
        return false;
    }
    float2 uv = float2(Random(lightIndex, pixel, sampleIndex, 0u),
        Random(lightIndex, pixel, sampleIndex, 1u)) - 0.5f;
    float3 toSample = light.Position + axisX * (uv.x * size.x) +
        axisY * (uv.y * size.y) - position;
    float distanceToSample = length(toSample);
    return distanceToSample > 1e-6f && TraceVisibility(position, normal,
        toSample / distanceToSample, distanceToSample, pixelWorldSize, visibility);
}

float AccumulateFinitePointVisibility(LightStruct light, uint lightIndex, uint2 pixel,
    float3 position, float3 normal, float pixelWorldSize)
{
    uint count = max(RT_SHADOW_QUALITY.FiniteEmitterSampleCount, 1u);
    float sum = 0.0f;
    [loop] for (uint sample = 0u; sample < count; ++sample) {
        float visibility;
        PointOrSpotSample(light, lightIndex, pixel, position, normal,
            pixelWorldSize, sample, false, false, visibility);
        sum += visibility;
    }
    return sum / float(count);
}

float AccumulateFiniteSpotVisibility(LightStruct light, uint lightIndex, uint2 pixel,
    float3 position, float3 normal, float pixelWorldSize)
{
    uint count = max(RT_SHADOW_QUALITY.FiniteEmitterSampleCount, 1u);
    float sum = 0.0f;
    [loop] for (uint sample = 0u; sample < count; ++sample) {
        float visibility;
        PointOrSpotSample(light, lightIndex, pixel, position, normal,
            pixelWorldSize, sample, true, true, visibility);
        sum += visibility;
    }
    return sum / float(count);
}

float AccumulateFiniteDirectionalVisibility(LightStruct light, uint lightIndex, uint2 pixel,
    float3 position, float3 normal, float pixelWorldSize)
{
    uint count = max(RT_SHADOW_QUALITY.FiniteEmitterSampleCount, 1u);
    float sum = 0.0f;
    [loop] for (uint sample = 0u; sample < count; ++sample) {
        float visibility;
        DirectionalSample(light, lightIndex, pixel, position, normal,
            pixelWorldSize, sample, visibility);
        sum += visibility;
    }
    return sum / float(count);
}

float AccumulateRectangleVisibility(LightStruct light, uint lightIndex, uint2 pixel,
    float3 position, float3 normal, float pixelWorldSize)
{
    uint count = max(RT_SHADOW_QUALITY.FiniteEmitterSampleCount, 1u);
    float sum = 0.0f;
    [loop] for (uint sample = 0u; sample < count; ++sample) {
        float visibility;
        RectangleSample(light, lightIndex, pixel, position, normal,
            pixelWorldSize, sample, true, visibility);
        sum += visibility;
    }
    return sum / float(count);
}

#endif
