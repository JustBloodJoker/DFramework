#ifndef NEURAL_SHADOW_VISIBILITY_DATA_HLSLI
#define NEURAL_SHADOW_VISIBILITY_DATA_HLSLI

#include "NeuralShadowWorkItemData.hlsli"

static const uint NS_OUTPUT_LAYOUT_VERSION = 1u;
static const uint NS_OUTPUT_VISIBILITY_ENCODING_R16_FLOAT = 1u;
static const uint NS_OUTPUT_VALIDITY_ENCODING_R8_UINT = 1u;
static const uint NS_OUTPUT_PRODUCER_BASELINE = 1u;
static const uint NS_OUTPUT_PRODUCER_RT_REFERENCE = 2u;
static const uint NS_OUTPUT_PRODUCER_NEURAL = 3u;
static const uint NS_OUTPUT_COMPLETION_INCOMPLETE = 0u;
static const uint NS_OUTPUT_COMPLETION_COMPLETE = 1u;
static const uint NS_OUTPUT_FALLBACK_POLICY = 1u;
static const uint NS_OUTPUT_LOOKUP_PATCH_BINARY_SEARCH = 1u;
static const uint NS_OUTPUT_MAX_LOOKUP_PROBES = 19u;

static const uint NS_OUTPUT_LOOKUP_FOUND = 0u;
static const uint NS_OUTPUT_LOOKUP_MISSING_PAIR = 1u;
static const uint NS_OUTPUT_LOOKUP_GENERATION_MISMATCH = 2u;
static const uint NS_OUTPUT_LOOKUP_INCOMPLETE = 3u;
static const uint NS_OUTPUT_LOOKUP_INCOMPATIBLE_LAYOUT = 4u;
static const uint NS_OUTPUT_LOOKUP_INVALID_PATCH = 5u;
static const uint NS_OUTPUT_LOOKUP_INVALID_WORK_RANGE = 6u;

static const uint NS_OUTPUT_SOURCE_COMPACT = 0u;
static const uint NS_OUTPUT_SOURCE_HARD_ATLAS = 1u;
static const uint NS_OUTPUT_SOURCE_FULLY_LIT = 2u;

static const uint NS_OUTPUT_VALIDITY_VALID = 0u;
static const uint NS_OUTPUT_VALIDITY_PADDED_OR_NO_RECEIVER = 1u;
static const uint NS_OUTPUT_VALIDITY_INVALID_LIGHT_IDENTITY = 2u;
static const uint NS_OUTPUT_VALIDITY_OUTSIDE_LIGHT_DOMAIN = 3u;
static const uint NS_OUTPUT_VALIDITY_MISSING_AS = 4u;
static const uint NS_OUTPUT_VALIDITY_UNSUPPORTED_LIGHT_TYPE = 5u;
static const uint NS_OUTPUT_VALIDITY_UNSUPPORTED_EMITTER_EXTENT = 6u;
static const uint NS_OUTPUT_VALIDITY_UNSUPPORTED_GEOMETRY = 7u;
static const uint NS_OUTPUT_VALIDITY_INVALID_RAY_INTERVAL = 8u;
static const uint NS_OUTPUT_VALIDITY_CAPACITY_OVERFLOW = 9u;
static const uint NS_OUTPUT_VALIDITY_GENERATION_MISMATCH = 10u;
static const uint NS_OUTPUT_VALIDITY_INCOMPLETE = 11u;

struct NSOutputHeader
{
    uint LayoutVersion;
    uint GenerationLow;
    uint GenerationHigh;
    uint VisibilityEncoding;
    uint ValidityEncoding;
    uint Producer;
    uint Completion;
    uint FallbackPolicy;
    uint WorkItemCount;
    uint OutputValueCount;
    uint ValuesPerWorkItem;
    uint VisibilityByteCount;
    uint ValidityByteCount;
    uint LookupKind;
    uint MaxLookupProbes;
    uint Padding0;
};

struct NSOutputLookupResult
{
    uint WorkItemIndex;
    uint OutputOffset;
    uint ProbeCount;
    uint Status;
};

struct NSOutputFallbackInput
{
    uint Available;
    uint GenerationLow;
    uint GenerationHigh;
    float Visibility;
};

struct NSOutputResolveResult
{
    float Visibility;
    uint Source;
    uint ValidityReason;
    uint WorkItemIndex;
};

bool NSOutputIsSupportedProducer(uint producer)
{
    return producer >= NS_OUTPUT_PRODUCER_BASELINE && producer <= NS_OUTPUT_PRODUCER_NEURAL;
}

bool NSOutputIsWorkForViewport(NSWorkDispatchHeader work, uint viewportWidth, uint viewportHeight)
{
    uint patchCountX = viewportWidth / NS_WORK_PATCH_WIDTH +
        (viewportWidth % NS_WORK_PATCH_WIDTH != 0u ? 1u : 0u);
    uint patchCountY = viewportHeight / NS_WORK_PATCH_HEIGHT +
        (viewportHeight % NS_WORK_PATCH_HEIGHT != 0u ? 1u : 0u);
    
    return viewportWidth != 0u && viewportHeight != 0u &&
        work.ScreenWidth == viewportWidth &&
        work.ScreenHeight == viewportHeight &&
        work.PatchCountX == patchCountX &&
        work.PatchCountY == patchCountY &&
        patchCountX != 0u && patchCountY <= NS_WORK_MAX_ITEMS / patchCountX;
}

bool NSOutputIsCompleteHeader(NSOutputHeader output, NSWorkDispatchHeader work, uint2 expectedGeneration)
{
    uint2 outputGeneration = uint2(output.GenerationLow, output.GenerationHigh);
    uint2 workGeneration = uint2(work.GenerationLow, work.GenerationHigh);
    
    return any(expectedGeneration != 0u) &&
        output.LayoutVersion == NS_OUTPUT_LAYOUT_VERSION &&
        all(outputGeneration == expectedGeneration) &&
        work.LayoutVersion == NS_WORK_LAYOUT_VERSION &&
        all(workGeneration == expectedGeneration) &&
        work.PatchWidth == NS_WORK_PATCH_WIDTH &&
        work.PatchHeight == NS_WORK_PATCH_HEIGHT &&
        work.WorkItemCount <= NS_WORK_MAX_ITEMS &&
        output.VisibilityEncoding == NS_OUTPUT_VISIBILITY_ENCODING_R16_FLOAT &&
        output.ValidityEncoding == NS_OUTPUT_VALIDITY_ENCODING_R8_UINT &&
        NSOutputIsSupportedProducer(output.Producer) &&
        output.Completion == NS_OUTPUT_COMPLETION_COMPLETE &&
        output.FallbackPolicy == NS_OUTPUT_FALLBACK_POLICY &&
        output.WorkItemCount == work.WorkItemCount &&
        output.WorkItemCount <= NS_WORK_MAX_ITEMS &&
        output.OutputValueCount == output.WorkItemCount * NS_WORK_VALUES_PER_ITEM &&
        output.OutputValueCount == work.OutputValueCount &&
        output.ValuesPerWorkItem == NS_WORK_VALUES_PER_ITEM &&
        output.VisibilityByteCount == output.OutputValueCount * 2u &&
        output.ValidityByteCount == output.OutputValueCount &&
        output.LookupKind == NS_OUTPUT_LOOKUP_PATCH_BINARY_SEARCH &&
        output.MaxLookupProbes == NS_OUTPUT_MAX_LOOKUP_PROBES &&
        output.Padding0 == 0u;
}

bool NSOutputIsCompleteForViewport(
    NSOutputHeader output, NSWorkDispatchHeader work, uint2 expectedGeneration, uint viewportWidth, uint viewportHeight)
{
    return NSOutputIsWorkForViewport(work, viewportWidth, viewportHeight) &&
        NSOutputIsCompleteHeader(output, work, expectedGeneration);
}

uint2 NSOutputComputeStoredPatchRange(NSWorkPatchRecord patch, uint workItemCount)
{
    uint start = min(patch.WorkItemOffset, workItemCount);
    uint storedCount = min(patch.UniquePairCount, workItemCount - start);
    return uint2(start, start + storedCount);
}

NSOutputLookupResult NSOutputFindWorkItem(
    uint screenTileIndex,
    uint lightIndex,
    NSWorkPatchRecord patch,
    uint workItemCount,
    StructuredBuffer<NSWorkItem> workItems)
{
    NSOutputLookupResult result;
    result.WorkItemIndex = NS_WORK_INVALID_INDEX;
    result.OutputOffset = NS_WORK_INVALID_INDEX;
    result.ProbeCount = 0u;
    result.Status = NS_OUTPUT_LOOKUP_MISSING_PAIR;
    uint2 range = NSOutputComputeStoredPatchRange(patch, workItemCount);
    uint low = range.x;
    uint high = range.y;
    [unroll]
    for (uint probe = 0u; probe < NS_OUTPUT_MAX_LOOKUP_PROBES; ++probe)
    {
        if (low >= high) {
            break;
        }

        uint middle = low + (high - low) / 2u;
        NSWorkItem candidate = workItems[middle];
        result.ProbeCount++;
        if (candidate.LightIndex < lightIndex) {
            low = middle + 1u;
        }
        else {
            high = middle;
        }
    }
    
    if (low < range.y)
    {
        NSWorkItem candidate = workItems[low];
        if (candidate.ScreenTileIndex == screenTileIndex && candidate.LightIndex == lightIndex)
        {
            result.WorkItemIndex = low;
            result.OutputOffset = candidate.OutputOffset;
            result.Status = NS_OUTPUT_LOOKUP_FOUND;
        }
    }
    return result;
}

NSOutputResolveResult NSOutputResolveVisibility(
    uint2 expectedGeneration,
    NSOutputLookupResult lookup,
    float compactVisibility,
    uint validityReason,
    NSOutputFallbackInput hardAtlasFallback)
{
    NSOutputResolveResult result;
    result.Visibility = 1.0f;
    result.Source = NS_OUTPUT_SOURCE_FULLY_LIT;
    result.ValidityReason = lookup.Status == NS_OUTPUT_LOOKUP_GENERATION_MISMATCH
        ? NS_OUTPUT_VALIDITY_GENERATION_MISMATCH
        : NS_OUTPUT_VALIDITY_INCOMPLETE;
    result.WorkItemIndex = lookup.WorkItemIndex;
    
    if (lookup.Status == NS_OUTPUT_LOOKUP_FOUND && validityReason == NS_OUTPUT_VALIDITY_VALID && isfinite(compactVisibility))
    {
        result.Visibility = saturate(compactVisibility);
        result.Source = NS_OUTPUT_SOURCE_COMPACT;
        result.ValidityReason = validityReason;
        return result;
    }
    
    if (lookup.Status == NS_OUTPUT_LOOKUP_FOUND) {
        result.ValidityReason = validityReason <= NS_OUTPUT_VALIDITY_INCOMPLETE
            ? validityReason
            : NS_OUTPUT_VALIDITY_INCOMPLETE;
    }
    
    if (hardAtlasFallback.Available != 0u && any(expectedGeneration != 0u) &&
        all(uint2(hardAtlasFallback.GenerationLow, hardAtlasFallback.GenerationHigh) == expectedGeneration) &&
        isfinite(hardAtlasFallback.Visibility))
    {
        result.Visibility = saturate(hardAtlasFallback.Visibility);
        result.Source = NS_OUTPUT_SOURCE_HARD_ATLAS;
    }
    return result;
}

static const uint NS_NEURAL_OUTPUT_LAYOUT_VERSION = 3u;
static const uint NS_NEURAL_OUTPUT_OUTPUT_LAYOUT_VERSION = 2u;
static const uint NS_NEURAL_OUTPUT_OUTPUT_VALUES = 4096u;
static const uint NS_NEURAL_OUTPUT_VALIDITY_VALID = 0u;

struct NSNeuralOutputGridWorkItem
{
    uint ScreenRegionIndex;
    uint LightIndex;
    uint ShadowMetaIndex;
    uint LowInputOffset;
    uint OutputOffset;
    uint Padding0;
    uint Padding1;
    uint Padding2;
};

struct NSNeuralOutputGridOutputHeader
{
    uint LayoutVersion;
    uint SpatialLayoutVersion;
    uint GenerationLow;
    uint GenerationHigh;
    uint ScreenWidth;
    uint ScreenHeight;
    uint RegionCountX;
    uint RegionCountY;
    uint WorkItemCount;
    uint ValuesPerWorkItem;
    uint VisibilityEncoding;
    uint ValidityEncoding;
    uint LookupKind;
    uint Completion;
    uint Padding0;
    uint Padding1;
};

bool NSNeuralOutputGridHeaderCompatible(NSNeuralOutputGridOutputHeader header)
{
    return header.LayoutVersion == NS_NEURAL_OUTPUT_OUTPUT_LAYOUT_VERSION &&
        header.SpatialLayoutVersion == NS_NEURAL_OUTPUT_LAYOUT_VERSION &&
        any(uint2(header.GenerationLow, header.GenerationHigh) != 0u) &&
        header.ScreenWidth != 0u && header.ScreenHeight != 0u &&
        header.RegionCountX == (header.ScreenWidth + 63u) / 64u &&
        header.RegionCountY == (header.ScreenHeight + 63u) / 64u &&
        header.ValuesPerWorkItem == NS_NEURAL_OUTPUT_OUTPUT_VALUES &&
        header.VisibilityEncoding == 1u && header.ValidityEncoding == 1u &&
        header.LookupKind == 1u && header.Completion == 1u;
}

uint NSNeuralOutputGridRegionIndex(uint2 pixel, uint regionCountX)
{
    return pixel.x / 64u + (pixel.y / 64u) * regionCountX;
}

uint NSNeuralOutputGridFindWorkItem(
    StructuredBuffer<NSNeuralOutputGridWorkItem> workItems,
    uint workCount,
    uint regionIndex,
    uint lightIndex)
{
    uint lower = 0u;
    uint upper = workCount;

    [loop] while (lower < upper)
    {
        uint middle = lower + (upper - lower) / 2u;
        NSNeuralOutputGridWorkItem item = workItems[middle];
        bool less = item.ScreenRegionIndex < regionIndex ||
            (item.ScreenRegionIndex == regionIndex && item.LightIndex < lightIndex);

        if (less) {
            lower = middle + 1u;
        }
        else {
            upper = middle;
        }
    }

    if (lower < workCount)
    {
        NSNeuralOutputGridWorkItem item = workItems[lower];
        if (item.ScreenRegionIndex == regionIndex && item.LightIndex == lightIndex) {
            return lower;
        }
    }

    return 0xffffffffu;
}


#endif
