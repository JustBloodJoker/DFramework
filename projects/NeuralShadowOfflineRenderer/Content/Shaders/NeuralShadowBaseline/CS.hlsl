#include "Utilits.hlsli"
#include "Structures.hlsli"
#include "NeuralShadowFeaturePacking.hlsli"
#include "NeuralShadowVisibilityData.hlsli"

static const uint NS_BASELINE_BATCH_CAPACITY = 1024u;
static const uint NS_BASELINE_PASS_INITIALIZE = 0u;
static const uint NS_BASELINE_PASS_PREPARE_FEATURES = 1u;
static const uint NS_BASELINE_PASS_RESOLVE_NEAREST = 2u;
static const uint NS_BASELINE_PASS_FINALIZE = 3u;
static const uint NS_BASELINE_PIXEL_PAIRS_PER_ITEM = NS_FEATURE_PIXELS_PER_PATCH / 2u;
static const uint NS_BASELINE_PIXEL_QUADS_PER_ITEM = NS_FEATURE_PIXELS_PER_PATCH / 4u;

struct NSBaselineDispatchParams
{
    uint ScreenWidth;
    uint ScreenHeight;
    uint AtlasWidth;
    uint AtlasHeight;
    uint LightCount;
    uint ShadowMetaCount;
    uint BatchCapacity;
    uint Padding;
    float CameraNear;
    float CameraFar;
    uint Padding0;
    uint Padding1;
    matrix InverseViewProjectionMatrix;
};

uint NSBaselinePackHalf2(float first, float second)
{
    return f32tof16(first) | (f32tof16(second) << 16u);
}

float NSBaselineLoadHalf(RWStructuredBuffer<uint> packedValues, uint elementIndex)
{
    uint word = packedValues[elementIndex >> 1u];
    uint bits = (elementIndex & 1u) == 0u ? word & 0xffffu : word >> 16u;
    return f16tof32(bits);
}

uint NSBaselinePackValidity4(uint first, uint second, uint third, uint fourth)
{
    return (first & 0xffu) | ((second & 0xffu) << 8u) |
        ((third & 0xffu) << 16u) | ((fourth & 0xffu) << 24u);
}


StructuredBuffer<NSWorkItem> WorkItems : register(t0);
StructuredBuffer<NSWorkDispatchHeader> WorkHeader : register(t1);
StructuredBuffer<LightStruct> Lights : register(t2);
StructuredBuffer<LightAtlasMeta> ShadowMetas : register(t3);
Texture2D<float> ShadowAtlas : register(t4);
Texture2D<float> DepthBuffer : register(t5);
Texture2D<float4> NormalBuffer : register(t6);
RWStructuredBuffer<uint> PixelFeatures : register(u0);
RWStructuredBuffer<uint> LightFeatures : register(u1);
RWStructuredBuffer<uint> OutputVisibility : register(u2);
RWStructuredBuffer<uint> OutputValidity : register(u3);
RWStructuredBuffer<NSOutputHeader> OutputHeader : register(u4);

cbuffer BaselineParams : register(b0)
{
    NSBaselineDispatchParams Params;
};

cbuffer BaselinePassConstants : register(b1)
{
    uint PassIndex;
    uint BatchBase;
};

uint2 ComputeGlobalPixel(NSWorkItem item, uint localPixelIndex)
{
    uint patchX = item.ScreenTileIndex % WorkHeader[0].PatchCountX;
    uint patchY = item.ScreenTileIndex / WorkHeader[0].PatchCountX;
    return uint2(
        patchX * NS_FEATURE_PATCH_WIDTH + localPixelIndex % NS_FEATURE_PATCH_WIDTH,
        patchY * NS_FEATURE_PATCH_HEIGHT + localPixelIndex / NS_FEATURE_PATCH_WIDTH);
}

bool TryLoadHardVisibility(NSWorkItem item, uint2 globalPixel, out float visibility, out bool insideDomain)
{
    visibility = 1.0f;
    insideDomain = false;
    if (item.LightIndex >= Params.LightCount || item.ShadowMetaIndex >= Params.ShadowMetaCount) {
        return false;
    }

    LightAtlasMeta meta = ShadowMetas[item.ShadowMetaIndex];
    if (meta.LightIndex != item.LightIndex || meta.AtlasWidth == 0u || meta.AtlasHeight == 0u ||
        meta.ScreenMaxU <= meta.ScreenMinU || meta.ScreenMaxV <= meta.ScreenMinV ||
        meta.AtlasOffsetX >= Params.AtlasWidth || meta.AtlasOffsetY >= Params.AtlasHeight ||
        meta.AtlasWidth > Params.AtlasWidth - meta.AtlasOffsetX ||
        meta.AtlasHeight > Params.AtlasHeight - meta.AtlasOffsetY) {
        return false;
    }

    float2 uv = (float2(globalPixel) + 0.5f) / float2(Params.ScreenWidth, Params.ScreenHeight);
    if (uv.x < meta.ScreenMinU || uv.x >= meta.ScreenMaxU || uv.y < meta.ScreenMinV || uv.y >= meta.ScreenMaxV) {
        return true;
    }

    insideDomain = true;
    float2 relativeUV = saturate((uv - float2(meta.ScreenMinU, meta.ScreenMinV)) /
        float2(meta.ScreenMaxU - meta.ScreenMinU, meta.ScreenMaxV - meta.ScreenMinV));
    uint2 minPixel = uint2(meta.AtlasOffsetX, meta.AtlasOffsetY);
    uint2 atlasPixel = min(
        minPixel + uint2(relativeUV * float2(meta.AtlasWidth, meta.AtlasHeight)),
        minPixel + uint2(meta.AtlasWidth - 1u, meta.AtlasHeight - 1u));
    visibility = saturate(ShadowAtlas.Load(int3(atlasPixel, 0)));
    return true;
}

NSPixelFeatures ExtractPixelFeatures(NSWorkItem item, uint localPixelIndex)
{
    NSPixelFeatures zeroOutput = (NSPixelFeatures)0;
    uint2 globalPixel = ComputeGlobalPixel(item, localPixelIndex);
    if (globalPixel.x >= Params.ScreenWidth || globalPixel.y >= Params.ScreenHeight ||
        item.LightIndex >= Params.LightCount || item.ShadowMetaIndex >= Params.ShadowMetaCount) {
        return zeroOutput;
    }

    float hardVisibility;
    bool insideDomain;
    if (!TryLoadHardVisibility(item, globalPixel, hardVisibility, insideDomain) || !insideDomain) {
        return zeroOutput;
    }
    float hardwareDepth = DepthBuffer.Load(int3(globalPixel, 0));
    NSPixelFeatureInput pixel = (NSPixelFeatureInput)0;
    pixel.HardVisibility = hardVisibility;
    pixel.ReceiverLinearDepth = (Params.CameraNear * Params.CameraFar) /
        (Params.CameraFar - hardwareDepth * (Params.CameraFar - Params.CameraNear));
    pixel.CameraNear = Params.CameraNear;
    pixel.CameraFar = Params.CameraFar;
    pixel.ReceiverNormal = NormalBuffer.Load(int3(globalPixel, 0)).xyz;
    pixel.ReceiverWorldPosition = ReconstructWorldPosition(
        (float2(globalPixel) + 0.5f) / float2(Params.ScreenWidth, Params.ScreenHeight),
        hardwareDepth,
        Params.InverseViewProjectionMatrix);
    pixel.ReceiverValid = isfinite(hardwareDepth) && hardwareDepth >= 0.0f && hardwareDepth < 1.0f ? 1u : 0u;
    LightStruct source = Lights[item.LightIndex];
    NSLightFeatureInput light = (NSLightFeatureInput)0;
    light.LightType = source.LightType;
    light.Position = source.Position;
    light.AttenuationRadius = source.AttenuationRadius;
    light.SourceRadius = source.SourceRadius;
    light.Direction = source.Direction;
    light.RectangleWidth = source.RectSize.x;
    light.RectangleHeight = source.RectSize.y;
    light.Rotation = source.Rotation;
    return NSFeatureExtractPixel(pixel, light);
}

NSLightFeatures ExtractLightFeatures(NSWorkItem item)
{
    NSLightFeatures zeroOutput = (NSLightFeatures)0;
    if (item.LightIndex >= Params.LightCount) {
        return zeroOutput;
    }
    LightStruct source = Lights[item.LightIndex];
    NSLightFeatureInput light = (NSLightFeatureInput)0;
    light.LightType = source.LightType;
    light.Position = source.Position;
    light.AttenuationRadius = source.AttenuationRadius;
    light.SourceRadius = source.SourceRadius;
    light.Direction = source.Direction;
    light.RectangleWidth = source.RectSize.x;
    light.RectangleHeight = source.RectSize.y;
    light.Rotation = source.Rotation;
    light.DirectionalAngularRadius = source.DirectionalAngularRadius;
    light.ShadowSoftness = source.ShadowSoftness;
    return NSFeatureExtractLight(light, Params.CameraFar);
}

void InitializeHeader()
{
    NSWorkDispatchHeader work = WorkHeader[0];
    NSOutputHeader output = (NSOutputHeader)0;
    output.LayoutVersion = NS_OUTPUT_LAYOUT_VERSION;
    output.GenerationLow = work.GenerationLow;
    output.GenerationHigh = work.GenerationHigh;
    output.VisibilityEncoding = NS_OUTPUT_VISIBILITY_ENCODING_R16_FLOAT;
    output.ValidityEncoding = NS_OUTPUT_VALIDITY_ENCODING_R8_UINT;
    output.Producer = NS_OUTPUT_PRODUCER_BASELINE;
    output.Completion = NS_OUTPUT_COMPLETION_INCOMPLETE;
    output.FallbackPolicy = NS_OUTPUT_FALLBACK_POLICY;
    output.WorkItemCount = min(work.WorkItemCount, NS_WORK_MAX_ITEMS);
    output.OutputValueCount = output.WorkItemCount * NS_WORK_VALUES_PER_ITEM;
    output.ValuesPerWorkItem = NS_WORK_VALUES_PER_ITEM;
    output.VisibilityByteCount = output.OutputValueCount * 2u;
    output.ValidityByteCount = output.OutputValueCount;
    output.LookupKind = NS_OUTPUT_LOOKUP_PATCH_BINARY_SEARCH;
    output.MaxLookupProbes = NS_OUTPUT_MAX_LOOKUP_PROBES;
    OutputHeader[0] = output;
}

void PrepareFeaturePair(uint pairIndex)
{
    uint localBatchIndex = pairIndex / NS_BASELINE_PIXEL_PAIRS_PER_ITEM;
    uint workItemIndex = BatchBase + localBatchIndex;
    NSWorkDispatchHeader header = WorkHeader[0];
    if (localBatchIndex >= Params.BatchCapacity || workItemIndex >= header.WorkItemCount) {
        return;
    }
    NSWorkItem item = WorkItems[workItemIndex];
    uint localPair = pairIndex % NS_BASELINE_PIXEL_PAIRS_PER_ITEM;
    uint firstPixel = localPair * 2u;
    if ((localPair & 1u) == 0u)
    {
        OutputValidity[(item.OutputOffset + firstPixel) >> 2u] = NSBaselinePackValidity4(
            NS_OUTPUT_VALIDITY_INCOMPLETE,
            NS_OUTPUT_VALIDITY_INCOMPLETE,
            NS_OUTPUT_VALIDITY_INCOMPLETE,
            NS_OUTPUT_VALIDITY_INCOMPLETE);
    }
    NSPixelFeatures first = ExtractPixelFeatures(item, firstPixel);
    NSPixelFeatures second = ExtractPixelFeatures(item, firstPixel + 1u);
    [unroll]
    for (uint channel = 0u; channel < NS_FEATURE_PIXEL_CHANNEL_COUNT; ++channel)
    {
        uint firstOffset = NSFeatureComputePixelTensorOffset(localBatchIndex, channel,
            uint2(firstPixel % NS_FEATURE_PATCH_WIDTH, firstPixel / NS_FEATURE_PATCH_WIDTH));
        PixelFeatures[firstOffset >> 1u] = NSBaselinePackHalf2(first.Values[channel], second.Values[channel]);
    }
    if (localPair == 0u)
    {
        NSLightFeatures light = ExtractLightFeatures(item);
        [unroll]
        for (uint pair = 0u; pair < NS_FEATURE_LIGHT_CHANNEL_COUNT / 2u; ++pair)
        {
            uint firstOffset = NSFeatureComputeLightTensorOffset(localBatchIndex, pair * 2u);
            LightFeatures[firstOffset >> 1u] = NSBaselinePackHalf2(
                light.Values[pair * 2u], light.Values[pair * 2u + 1u]);
        }
    }
}

uint ComputeValidity(NSWorkItem item, uint localPixelIndex, float receiverValid)
{
    uint2 globalPixel = ComputeGlobalPixel(item, localPixelIndex);
    if (globalPixel.x >= Params.ScreenWidth || globalPixel.y >= Params.ScreenHeight) {
        return NS_OUTPUT_VALIDITY_PADDED_OR_NO_RECEIVER;
    }

    if (item.LightIndex >= Params.LightCount || item.ShadowMetaIndex >= Params.ShadowMetaCount) {
        return NS_OUTPUT_VALIDITY_INVALID_LIGHT_IDENTITY;
    }

    LightAtlasMeta meta = ShadowMetas[item.ShadowMetaIndex];
    if (meta.LightIndex != item.LightIndex) {
        return NS_OUTPUT_VALIDITY_INVALID_LIGHT_IDENTITY;
    }

    int lightType = Lights[item.LightIndex].LightType;
    if (lightType < NS_FEATURE_LIGHT_TYPE_POINT_ENUM || lightType > NS_FEATURE_LIGHT_TYPE_RECTANGLE_ENUM) {
        return NS_OUTPUT_VALIDITY_UNSUPPORTED_LIGHT_TYPE;
    }

    float hardVisibility;
    bool insideDomain;
    if (!TryLoadHardVisibility(item, globalPixel, hardVisibility, insideDomain)) {
        return NS_OUTPUT_VALIDITY_INVALID_LIGHT_IDENTITY;
    }

    if (!insideDomain) {
        return NS_OUTPUT_VALIDITY_OUTSIDE_LIGHT_DOMAIN;
    }

    return receiverValid >= 0.5f ? NS_OUTPUT_VALIDITY_VALID : NS_OUTPUT_VALIDITY_PADDED_OR_NO_RECEIVER;
}

void ResolvePixelQuad(uint quadIndex)
{
    uint localBatchIndex = quadIndex / NS_BASELINE_PIXEL_QUADS_PER_ITEM;
    uint workItemIndex = BatchBase + localBatchIndex;
    NSWorkDispatchHeader header = WorkHeader[0];
    if (localBatchIndex >= Params.BatchCapacity || workItemIndex >= header.WorkItemCount) {
        return;
    }

    NSWorkItem item = WorkItems[workItemIndex];
    uint firstPixel = (quadIndex % NS_BASELINE_PIXEL_QUADS_PER_ITEM) * 4u;
    float visibility[4];
    uint validity[4];
    [unroll]
    for (uint pixel = 0u; pixel < 4u; ++pixel)
    {
        uint localPixel = firstPixel + pixel;
        uint hardOffset = NSFeatureComputePixelTensorOffset(
            localBatchIndex,
            NS_FEATURE_PIXEL_HARD_VISIBILITY,
            uint2(localPixel % NS_FEATURE_PATCH_WIDTH, localPixel / NS_FEATURE_PATCH_WIDTH));
        uint validOffset = NSFeatureComputePixelTensorOffset(
            localBatchIndex,
            NS_FEATURE_PIXEL_RECEIVER_VALID,
            uint2(localPixel % NS_FEATURE_PATCH_WIDTH, localPixel / NS_FEATURE_PATCH_WIDTH));
        visibility[pixel] = NSBaselineLoadHalf(PixelFeatures, hardOffset);
        float receiverValid = NSBaselineLoadHalf(PixelFeatures, validOffset);
        validity[pixel] = ComputeValidity(item, localPixel, receiverValid);
        if (validity[pixel] != NS_OUTPUT_VALIDITY_VALID) visibility[pixel] = 1.0f;
    }
    uint outputOffset = item.OutputOffset + firstPixel;
    OutputVisibility[outputOffset >> 1u] = NSBaselinePackHalf2(visibility[0], visibility[1]);
    OutputVisibility[(outputOffset >> 1u) + 1u] = NSBaselinePackHalf2(visibility[2], visibility[3]);
    OutputValidity[outputOffset >> 2u] = NSBaselinePackValidity4(
        validity[0], validity[1], validity[2], validity[3]);
}

void FinalizeHeader()
{
    NSWorkDispatchHeader work = WorkHeader[0];
    NSOutputHeader output = OutputHeader[0];
    bool compatible = Params.BatchCapacity == NS_BASELINE_BATCH_CAPACITY &&
        work.LayoutVersion == NS_WORK_LAYOUT_VERSION &&
        any(uint2(work.GenerationLow, work.GenerationHigh) != 0u) &&
        work.WorkItemCount <= NS_WORK_MAX_ITEMS &&
        work.OutputValueCount == work.WorkItemCount * NS_WORK_VALUES_PER_ITEM;
    output.Completion = compatible ? NS_OUTPUT_COMPLETION_COMPLETE : NS_OUTPUT_COMPLETION_INCOMPLETE;
    OutputHeader[0] = output;
}

[numthreads(64, 1, 1)]
void CS(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    if (PassIndex == NS_BASELINE_PASS_INITIALIZE)
    {
        if (dispatchThreadId.x == 0u) InitializeHeader();
        return;
    }
    if (PassIndex == NS_BASELINE_PASS_FINALIZE)
    {
        if (dispatchThreadId.x == 0u) FinalizeHeader();
        return;
    }
    if (PassIndex == NS_BASELINE_PASS_PREPARE_FEATURES) PrepareFeaturePair(dispatchThreadId.x);
    else if (PassIndex == NS_BASELINE_PASS_RESOLVE_NEAREST) ResolvePixelQuad(dispatchThreadId.x);
}
