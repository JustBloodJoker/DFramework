#include "Structures.hlsli"
#include "NeuralShadowWorkItemData.hlsli"

StructuredBuffer<Cluster> CoarseTiles : register(t0);
StructuredBuffer<LightAtlasMeta> ShadowMetas : register(t1);
RWStructuredBuffer<NSWorkPatchRecord> PatchRecords : register(u0);
RWStructuredBuffer<NSWorkItem> WorkItems : register(u1);
RWStructuredBuffer<NSWorkDispatchHeader> DispatchHeader : register(u2);

cbuffer GenerationParams : register(b0)
{
    NSWorkGenerationParams Params;
};

cbuffer PassConstants : register(b1)
{
    uint PassIndex;
};

static const uint NS_WORK_PASS_COUNT = 0u;
static const uint NS_WORK_PASS_SCAN = 1u;
static const uint NS_WORK_PASS_WRITE = 2u;
static const uint NS_WORK_THREADS_X = 64u;

uint ComputeCoarseTileCoordinate(uint pixel, uint extent, uint tileCount)
{
    if (extent == 0u || tileCount == 0u) {
        return 0u;
    }
    uint clampedPixel = min(pixel, extent - 1u);
    return min((clampedPixel * tileCount) / extent, tileCount - 1u);
}

uint4 ComputePatchCoarseBounds(uint screenTileIndex)
{
    uint patchX = screenTileIndex % Params.PatchCountX;
    uint patchY = screenTileIndex / Params.PatchCountX;

    uint minPixelX = patchX * NS_WORK_PATCH_WIDTH;
    uint minPixelY = patchY * NS_WORK_PATCH_HEIGHT;
    uint maxPixelX = min(minPixelX + NS_WORK_PATCH_WIDTH - 1u, Params.ScreenWidth - 1u);
    uint maxPixelY = min(minPixelY + NS_WORK_PATCH_HEIGHT - 1u, Params.ScreenHeight - 1u);

    return uint4(
        ComputeCoarseTileCoordinate(minPixelX, Params.ScreenWidth, Params.CoarseGridWidth),
        ComputeCoarseTileCoordinate(minPixelY, Params.ScreenHeight, Params.CoarseGridHeight),
        ComputeCoarseTileCoordinate(maxPixelX, Params.ScreenWidth, Params.CoarseGridWidth),
        ComputeCoarseTileCoordinate(maxPixelY, Params.ScreenHeight, Params.CoarseGridHeight));
}

uint FindFirstLightAfter(Cluster tile, uint previousLight, bool hasPrevious)
{
    [loop]
    for (uint i = 0u; i < tile.Count; ++i)
    {
        uint lightIndex = tile.LightIndices[i];
        if (!hasPrevious || lightIndex > previousLight) {
            return lightIndex;
        }
    }

    return NS_WORK_INVALID_INDEX;
}

uint FindNextUnionLight(uint4 bounds, uint previousLight, bool hasPrevious, out uint occurrences)
{
    uint nextLight = NS_WORK_INVALID_INDEX;
    occurrences = 0u;

    [loop]
    for (uint tileY = bounds.y; tileY <= bounds.w; ++tileY)
    {
        [loop]
        for (uint tileX = bounds.x; tileX <= bounds.z; ++tileX)
        {
            uint tileIndex = tileX + tileY * Params.CoarseGridWidth;
            uint candidate = FindFirstLightAfter(CoarseTiles[tileIndex], previousLight, hasPrevious);
            nextLight = min(nextLight, candidate);
        }
    }

    if (nextLight == NS_WORK_INVALID_INDEX) {
        return nextLight;
    }

    [loop]
    for (uint occurrenceTileY = bounds.y; occurrenceTileY <= bounds.w; ++occurrenceTileY)
    {
        [loop]
        for (uint occurrenceTileX = bounds.x; occurrenceTileX <= bounds.z; ++occurrenceTileX)
        {
            uint tileIndex = occurrenceTileX + occurrenceTileY * Params.CoarseGridWidth;
            uint candidate = FindFirstLightAfter(CoarseTiles[tileIndex], previousLight, hasPrevious);
            occurrences += candidate == nextLight ? 1u : 0u;
        }
    }

    return nextLight;
}

uint FindShadowMetaIndex(uint lightIndex)
{
    uint foundIndex = NS_WORK_INVALID_INDEX;
    uint matchCount = 0u;
    [loop]
    for (uint metaIndex = 0u; metaIndex < Params.ShadowMetaCount; ++metaIndex)
    {
        if (ShadowMetas[metaIndex].LightIndex == lightIndex)
        {
            foundIndex = metaIndex;
            matchCount++;
        }
    }
    return matchCount == 1u ? foundIndex : NS_WORK_INVALID_INDEX;
}

void CountPatch(uint screenTileIndex)
{
    uint4 bounds = ComputePatchCoarseBounds(screenTileIndex);
    uint uniqueCount = 0u;
    uint duplicateCount = 0u;
    uint previousLight = 0u;
    bool hasPrevious = false;

    [loop]
    while (true)
    {
        uint occurrences = 0u;
        uint lightIndex = FindNextUnionLight(bounds, previousLight, hasPrevious, occurrences);
        if (lightIndex == NS_WORK_INVALID_INDEX) {
            break;
        }
        uniqueCount++;
        duplicateCount += occurrences > 0u ? occurrences - 1u : 0u;
        previousLight = lightIndex;
        hasPrevious = true;
    }

    NSWorkPatchRecord record;
    record.UniquePairCount = uniqueCount;
    record.DuplicateCandidateCount = duplicateCount;
    record.WorkItemOffset = 0u;
    record.Padding0 = 0u;
    PatchRecords[screenTileIndex] = record;
}

uint SaturatingAdd(uint left, uint right, inout uint overflowFlags)
{
    if (right > NS_WORK_INVALID_INDEX - left)
    {
        overflowFlags |= NS_WORK_OVERFLOW_REQUIRED_COUNT_SATURATED;
        return NS_WORK_INVALID_INDEX;
    }
    return left + right;
}

void ScanPatchCounts()
{
    NSWorkDispatchHeader header;
    header.LayoutVersion = NS_WORK_LAYOUT_VERSION;
    header.GenerationLow = Params.GenerationLow;
    header.GenerationHigh = Params.GenerationHigh;
    header.ScreenWidth = Params.ScreenWidth;
    header.ScreenHeight = Params.ScreenHeight;
    header.PatchWidth = NS_WORK_PATCH_WIDTH;
    header.PatchHeight = NS_WORK_PATCH_HEIGHT;
    header.PatchCountX = Params.PatchCountX;
    header.PatchCountY = Params.PatchCountY;
    header.WorkItemCount = 0u;
    header.RequiredWorkItemCount = 0u;
    header.WorkItemCapacity = NS_WORK_MAX_ITEMS;
    header.OutputValueCount = 0u;
    header.OutputValueCapacity = NS_WORK_MAX_OUTPUT_VALUES;
    header.LookupSlotCapacity = NS_WORK_LOOKUP_SLOT_CAPACITY;
    header.DroppedWorkItemCount = 0u;
    header.DuplicateCandidateCount = 0u;
    header.RuntimeFallbackCount = 0u;
    header.OverflowFlags = NS_WORK_OVERFLOW_NONE;
    header.OverflowPolicy = NS_WORK_OVERFLOW_POLICY;

    if (Params.ActiveShadowWork == 0u)
    {
        DispatchHeader[0] = header;
        return;
    }

    uint patchCount = Params.PatchCountX * Params.PatchCountY;
    uint scannedPatchCount = min(patchCount, NS_WORK_MAX_ITEMS);
    uint requiredCount = 0u;
    uint duplicateCount = 0u;

    [loop]
    for (uint patchIndex = 0u; patchIndex < scannedPatchCount; ++patchIndex)
    {
        NSWorkPatchRecord record = PatchRecords[patchIndex];
        record.WorkItemOffset = min(requiredCount, NS_WORK_MAX_ITEMS);

        PatchRecords[patchIndex] = record;

        requiredCount = SaturatingAdd(requiredCount, record.UniquePairCount, header.OverflowFlags);
        duplicateCount = SaturatingAdd(duplicateCount, record.DuplicateCandidateCount, header.OverflowFlags);
    }

    uint writtenCount = min(requiredCount, NS_WORK_MAX_ITEMS);
    if (patchCount > scannedPatchCount)
    {
        requiredCount = NS_WORK_INVALID_INDEX;
        header.OverflowFlags |= NS_WORK_OVERFLOW_ITEM_CAPACITY;
        header.OverflowFlags |= NS_WORK_OVERFLOW_REQUIRED_COUNT_SATURATED;
    }

    [loop]
    for (uint coarseTileIndex = 0u; coarseTileIndex < Params.CoarseTileCount; ++coarseTileIndex)
    {
        Cluster tile = CoarseTiles[coarseTileIndex];
        if (tile.AffectingLightCount > tile.Count)
        {
            header.OverflowFlags |= NS_WORK_OVERFLOW_UPSTREAM_TILE_LIST;
        }
    }

    header.RequiredWorkItemCount = requiredCount;
    header.WorkItemCount = writtenCount;
    header.OutputValueCount = header.WorkItemCount * NS_WORK_VALUES_PER_ITEM;
    header.DroppedWorkItemCount = requiredCount - header.WorkItemCount;
    header.DuplicateCandidateCount = duplicateCount;
    header.RuntimeFallbackCount = header.DroppedWorkItemCount;
    if (requiredCount > NS_WORK_MAX_ITEMS)
    {
        header.OverflowFlags |= NS_WORK_OVERFLOW_ITEM_CAPACITY;
    }
    DispatchHeader[0] = header;
}

void SaturatingIncrementRuntimeFallback()
{
    uint previousValue = 0u;
    InterlockedAdd(DispatchHeader[0].RuntimeFallbackCount, 1u, previousValue);
    if (previousValue == NS_WORK_INVALID_INDEX)
    {
        InterlockedMax(DispatchHeader[0].RuntimeFallbackCount, NS_WORK_INVALID_INDEX);
        InterlockedOr(DispatchHeader[0].OverflowFlags, NS_WORK_OVERFLOW_REQUIRED_COUNT_SATURATED);
    }
}

void WritePatch(uint screenTileIndex)
{
    NSWorkPatchRecord record = PatchRecords[screenTileIndex];
    if (record.WorkItemOffset >= NS_WORK_MAX_ITEMS) {
        return;
    }

    uint4 bounds = ComputePatchCoarseBounds(screenTileIndex);
    uint previousLight = 0u;
    bool hasPrevious = false;
    uint localIndex = 0u;

    [loop]
    while (true)
    {
        uint occurrences = 0u;
        uint lightIndex = FindNextUnionLight(bounds, previousLight, hasPrevious, occurrences);
        if (lightIndex == NS_WORK_INVALID_INDEX) {
            break;
        }

        uint workItemIndex = record.WorkItemOffset + localIndex;
        if (workItemIndex < NS_WORK_MAX_ITEMS)
        {
            uint shadowMetaIndex = lightIndex < Params.LightCount ? FindShadowMetaIndex(lightIndex) : NS_WORK_INVALID_INDEX;
            NSWorkItem item;
            item.ScreenTileIndex = screenTileIndex;
            item.LightIndex = lightIndex;
            item.ShadowMetaIndex = shadowMetaIndex;
            item.OutputOffset = workItemIndex * NS_WORK_VALUES_PER_ITEM;
            WorkItems[workItemIndex] = item;

            if (shadowMetaIndex == NS_WORK_INVALID_INDEX)
            {
                SaturatingIncrementRuntimeFallback();
                InterlockedOr( DispatchHeader[0].OverflowFlags, NS_WORK_OVERFLOW_INVALID_SHADOW_METADATA);
            }
        }

        localIndex++;
        previousLight = lightIndex;
        hasPrevious = true;
    }
}

[numthreads(NS_WORK_THREADS_X, 1, 1)]
void CS(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    uint patchCount = Params.PatchCountX * Params.PatchCountY;
    uint boundedPatchCount = min(patchCount, NS_WORK_MAX_ITEMS);

    if (PassIndex == NS_WORK_PASS_SCAN)
    {
        if (dispatchThreadId.x == 0u) ScanPatchCounts();
        return;
    }

    if (Params.ActiveShadowWork == 0u || dispatchThreadId.x >= boundedPatchCount) {
        return;
    }

    if (PassIndex == NS_WORK_PASS_COUNT) CountPatch(dispatchThreadId.x);
    else if (PassIndex == NS_WORK_PASS_WRITE) WritePatch(dispatchThreadId.x);
}
