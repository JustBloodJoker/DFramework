#include "SameShadersStructs.hlsli"
#include "Structures.hlsli"
#include "Utilits.hlsli"
#include "NeuralShadowRayTracingData.hlsli"
#include "NeuralShadowVisibilityData.hlsli"
#include "NeuralShadowFeaturePacking.hlsli"

struct NSRTTargetParams
{
    float4x4 InverseViewProjectionMatrix;
    NSRTQualityMetadata Quality;
    uint PassIndex;
    uint WorkItemBase;
    uint DispatchWorkItemCount;
    uint LightCount;
    uint GeometryInvalidReason;
    uint OutputWidth;
    uint OutputHeight;
    uint LayoutVersion;
};

ConstantBuffer<NSRTTargetParams> Params : register(b0);
RaytracingAccelerationStructure SceneBVH : register(t0);
StructuredBuffer<LightStruct> Lights : register(t1);
StructuredBuffer<NSWorkItem> WorkItems : register(t2);
StructuredBuffer<NSWorkDispatchHeader> WorkHeader : register(t3);
Texture2D<float> DepthBuffer : register(t4);
Texture2D<float4> NormalBuffer : register(t5);
StructuredBuffer<uint> PixelFeatures : register(t209);
RWStructuredBuffer<uint> OutputVisibility : register(u0);
RWStructuredBuffer<uint> OutputValidity : register(u1);
RWStructuredBuffer<NSOutputHeader> OutputHeader : register(u2);

#define RT_SHADOW_QUALITY Params.Quality
#define RT_SHADOW_OUTPUT_WIDTH Params.OutputWidth
#define RT_SHADOW_OUTPUT_HEIGHT Params.OutputHeight
#define RT_SHADOW_INVERSE_VIEW_PROJECTION Params.InverseViewProjectionMatrix
#include "../ShadowCommon/RTShadowSampling.hlsli"

static const uint NS_RT_TARGET_PASS_INITIALIZE = 0u;
static const uint NS_RT_TARGET_PASS_TRACE = 1u;
static const uint NS_RT_TARGET_PASS_FINALIZE = 2u;

void EvaluatePixel(uint workIndex, NSWorkItem item, uint localPixel, out float visibility, out uint validity)
{
    visibility = 1.0f;
    validity = NS_OUTPUT_VALIDITY_PADDED_OR_NO_RECEIVER;
    uint patchX = item.ScreenTileIndex % WorkHeader[0].PatchCountX;
    uint patchY = item.ScreenTileIndex / WorkHeader[0].PatchCountX;
    uint2 pixel = uint2(patchX * NS_WORK_PATCH_WIDTH + localPixel % NS_WORK_PATCH_WIDTH,
        patchY * NS_WORK_PATCH_HEIGHT + localPixel / NS_WORK_PATCH_WIDTH);
    if (pixel.x >= Params.OutputWidth || pixel.y >= Params.OutputHeight) {
        return;
    }
    uint receiverOffset = (workIndex * NS_FEATURE_PIXEL_CHANNEL_COUNT +
        NS_FEATURE_PIXEL_RECEIVER_VALID) * NS_WORK_VALUES_PER_ITEM + localPixel;
    uint receiverWord = PixelFeatures[receiverOffset >> 1u];
    float receiverValid = f16tof32((receiverOffset & 1u) != 0u ? receiverWord >> 16u : receiverWord & 0xffffu);
    if (receiverValid < 0.5f) {
        return;
    }
    float depth = DepthBuffer.Load(int3(pixel, 0));
    float3 shadingNormal;
    if (!isfinite(depth) || depth < 0.0f || depth >= 1.0f) {
        return;
    }
    if (!TryNormalize(NormalBuffer.Load(int3(pixel, 0)).xyz, shadingNormal))
    {
        validity = NS_OUTPUT_VALIDITY_INVALID_RAY_INTERVAL;
        return;
    }
    if (item.LightIndex >= Params.LightCount) { validity = NS_OUTPUT_VALIDITY_INVALID_LIGHT_IDENTITY; return; }
    LightStruct light = Lights[item.LightIndex];
    float3 position = WorldPosition(pixel);
    if (Params.GeometryInvalidReason != NS_OUTPUT_VALIDITY_VALID) { validity = Params.GeometryInvalidReason; return; }
    if (light.LightType > LIGHT_RECT_LIGHT_ENUM_VALUE) { validity = NS_OUTPUT_VALIDITY_UNSUPPORTED_LIGHT_TYPE; return; }
    float pixelWorldSize;
    float3 receiverNormal = ReceiverNormal(pixel, shadingNormal, pixelWorldSize);
    float softness = isfinite(light.ShadowSoftness) ? clamp(light.ShadowSoftness, 0.0f, 4.0f) : 1.0f;
    float2 rectangleSize = light.RectSize * softness;
    if (light.LightType == LIGHT_RECT_LIGHT_ENUM_VALUE &&
        (!all(isfinite(rectangleSize)) || rectangleSize.x <= 0.0f || rectangleSize.y <= 0.0f))
    {
        validity = NS_OUTPUT_VALIDITY_UNSUPPORTED_EMITTER_EXTENT;
        return;
    }
    bool finite = (light.LightType == LIGHT_DIRECTIONAL_LIGHT_ENUM_VALUE && light.DirectionalAngularRadius * softness > 0.0f) ||
        ((light.LightType == LIGHT_POINT_LIGHT_ENUM_VALUE || light.LightType == LIGHT_SPOT_LIGHT_ENUM_VALUE) && light.SourceRadius * softness > 0.0f) ||
        (light.LightType == LIGHT_RECT_LIGHT_ENUM_VALUE && rectangleSize.x > 0.0f && rectangleSize.y > 0.0f);
    uint sampleCount = finite ? Params.Quality.FiniteEmitterSampleCount : Params.Quality.DeltaEmitterSampleCount;
    float sum = 0.0f;
    [loop]
    for (uint sample = 0u; sample < sampleCount; ++sample)
    {
        float sampleVisibility = 1.0f;
        bool traced = false;
        if (light.LightType == LIGHT_POINT_LIGHT_ENUM_VALUE)
            traced = PointOrSpotSample(light, item.LightIndex, pixel, position, receiverNormal, pixelWorldSize, sample, false, false, sampleVisibility);
        else if (light.LightType == LIGHT_SPOT_LIGHT_ENUM_VALUE)
            traced = PointOrSpotSample(light, item.LightIndex, pixel, position, receiverNormal, pixelWorldSize, sample, true, false, sampleVisibility);
        else if (light.LightType == LIGHT_DIRECTIONAL_LIGHT_ENUM_VALUE)
            traced = DirectionalSample(light, item.LightIndex, pixel, position, receiverNormal, pixelWorldSize, sample, sampleVisibility);
        else if (light.LightType == LIGHT_RECT_LIGHT_ENUM_VALUE)
            traced = RectangleSample(light, item.LightIndex, pixel, position, receiverNormal, pixelWorldSize, sample, false, sampleVisibility);
        if (!traced) { validity = light.LightType == LIGHT_RECT_LIGHT_ENUM_VALUE
            ? NS_OUTPUT_VALIDITY_INVALID_RAY_INTERVAL : NS_OUTPUT_VALIDITY_OUTSIDE_LIGHT_DOMAIN; return; }
        sum += sampleVisibility;
    }
    if (sampleCount == 0u) { validity = NS_OUTPUT_VALIDITY_INVALID_RAY_INTERVAL; return; }
    visibility = saturate(sum / float(sampleCount));
    validity = NS_OUTPUT_VALIDITY_VALID;
}

void InitializeHeader()
{
    NSOutputHeader output = (NSOutputHeader)0;
    output.LayoutVersion = NS_OUTPUT_LAYOUT_VERSION;
    output.GenerationLow = Params.Quality.GenerationLow;
    output.GenerationHigh = Params.Quality.GenerationHigh;
    output.VisibilityEncoding = NS_OUTPUT_VISIBILITY_ENCODING_R16_FLOAT;
    output.ValidityEncoding = NS_OUTPUT_VALIDITY_ENCODING_R8_UINT;
    output.Producer = NS_OUTPUT_PRODUCER_RT_REFERENCE;
    output.Completion = NS_OUTPUT_COMPLETION_INCOMPLETE;
    output.FallbackPolicy = NS_OUTPUT_FALLBACK_POLICY;
    output.WorkItemCount = WorkHeader[0].WorkItemCount;
    output.OutputValueCount = WorkHeader[0].OutputValueCount;
    output.ValuesPerWorkItem = NS_WORK_VALUES_PER_ITEM;
    output.VisibilityByteCount = output.OutputValueCount * 2u;
    output.ValidityByteCount = output.OutputValueCount;
    output.LookupKind = NS_OUTPUT_LOOKUP_PATCH_BINARY_SEARCH;
    output.MaxLookupProbes = NS_OUTPUT_MAX_LOOKUP_PROBES;
    OutputHeader[0] = output;
}

[shader("raygeneration")]
void RayGen()
{
    uint3 dispatch = DispatchRaysIndex();
    if (Params.PassIndex == NS_RT_TARGET_PASS_INITIALIZE)
    {
        if (all(dispatch == 0u)) InitializeHeader();
        return;
    }
    if (Params.PassIndex == NS_RT_TARGET_PASS_FINALIZE)
    {
        if (all(dispatch == 0u) && Params.LayoutVersion == NS_OUTPUT_LAYOUT_VERSION &&
            Params.OutputWidth == WorkHeader[0].ScreenWidth && Params.OutputHeight == WorkHeader[0].ScreenHeight &&
            WorkHeader[0].LayoutVersion == NS_WORK_LAYOUT_VERSION &&
            WorkHeader[0].GenerationLow == Params.Quality.GenerationLow &&
            WorkHeader[0].GenerationHigh == Params.Quality.GenerationHigh &&
            WorkHeader[0].PatchWidth == NS_WORK_PATCH_WIDTH &&
            WorkHeader[0].PatchHeight == NS_WORK_PATCH_HEIGHT &&
            WorkHeader[0].WorkItemCount <= NS_WORK_MAX_ITEMS &&
            WorkHeader[0].OutputValueCount == WorkHeader[0].WorkItemCount * NS_WORK_VALUES_PER_ITEM &&
            WorkHeader[0].OutputValueCount <= WorkHeader[0].OutputValueCapacity &&
            WorkHeader[0].DroppedWorkItemCount == 0u && WorkHeader[0].RuntimeFallbackCount == 0u &&
            Params.Quality.QualityMode == NS_RT_QUALITY_MODE_OFFLINE_GROUND_TRUTH &&
            Params.Quality.FiniteEmitterSampleCount >= NS_RT_OFFLINE_MIN_FINITE_SPP &&
            Params.Quality.FiniteEmitterSampleCount <= NS_RT_OFFLINE_MAX_FINITE_SPP && Params.Quality.DenoisingPolicy == 0u &&
            WorkHeader[0].OverflowFlags == 0u)
            OutputHeader[0].Completion = NS_OUTPUT_COMPLETION_COMPLETE;
        return;
    }
    if (Params.PassIndex != NS_RT_TARGET_PASS_TRACE || dispatch.x >= 4u || dispatch.y >= 16u ||
        dispatch.z >= Params.DispatchWorkItemCount) return;
    uint workIndex = Params.WorkItemBase + dispatch.z;
    if (workIndex >= WorkHeader[0].WorkItemCount) {
        return;
    }
    NSWorkItem item = WorkItems[workIndex];
    uint firstLocal = dispatch.y * 16u + dispatch.x * 4u;
    float values[4];
    uint reasons[4];
    [unroll] for (uint i = 0u; i < 4u; ++i)
    {
        EvaluatePixel(workIndex, item, firstLocal + i, values[i], reasons[i]);
    }
    uint outputBase = item.OutputOffset + firstLocal;
    OutputVisibility[outputBase / 2u] = f32tof16(values[0]) | (f32tof16(values[1]) << 16u);
    OutputVisibility[outputBase / 2u + 1u] = f32tof16(values[2]) | (f32tof16(values[3]) << 16u);
    OutputValidity[outputBase / 4u] = reasons[0] | (reasons[1] << 8u) | (reasons[2] << 16u) | (reasons[3] << 24u);
}
