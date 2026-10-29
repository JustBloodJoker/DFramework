#include "pch.h"

#include <ShadowData/NeuralShadowData.h>

namespace FDW_SD::Baseline {

NSNearestAtlasAddress ComputeNearestAtlasAddress(const LightAtlasMeta& meta,std::uint32_t screenX,std::uint32_t screenY,std::uint32_t screenWidth,std::uint32_t screenHeight,std::uint32_t atlasWidth,std::uint32_t atlasHeight) {
	NSNearestAtlasAddress result{};
	
	if (screenWidth == 0u || screenHeight == 0u || screenX >= screenWidth || screenY >= screenHeight ||
		meta.AtlasWidth == 0u || meta.AtlasHeight == 0u ||
		meta.ScreenMaxU <= meta.ScreenMinU || meta.ScreenMaxV <= meta.ScreenMinV ||
		meta.AtlasOffsetX >= atlasWidth || meta.AtlasOffsetY >= atlasHeight ||
		meta.AtlasWidth > atlasWidth - meta.AtlasOffsetX ||
		meta.AtlasHeight > atlasHeight - meta.AtlasOffsetY) {
	
		return result;
	}

	const float u = (static_cast<float>(screenX) + 0.5f) / static_cast<float>(screenWidth);
	const float v = (static_cast<float>(screenY) + 0.5f) / static_cast<float>(screenHeight);
	
	if (u < meta.ScreenMinU || u >= meta.ScreenMaxU || v < meta.ScreenMinV || v >= meta.ScreenMaxV) return result;
	
	const float relativeU = std::clamp((u - meta.ScreenMinU) / (meta.ScreenMaxU - meta.ScreenMinU), 0.0f, 1.0f);
	const float relativeV = std::clamp((v - meta.ScreenMinV) / (meta.ScreenMaxV - meta.ScreenMinV), 0.0f, 1.0f);
	
	result.X = std::min(meta.AtlasOffsetX + static_cast<std::uint32_t>(relativeU * static_cast<float>(meta.AtlasWidth)),meta.AtlasOffsetX + meta.AtlasWidth - 1u);
	result.Y = std::min(meta.AtlasOffsetY + static_cast<std::uint32_t>(relativeV * static_cast<float>(meta.AtlasHeight)),meta.AtlasOffsetY + meta.AtlasHeight - 1u);
	result.Valid = 1u;
	
	return result;
}

std::uint32_t PackHalf2(std::uint16_t first, std::uint16_t second) {
	return static_cast<std::uint32_t>(first) | (static_cast<std::uint32_t>(second) << 16u);
}

std::uint32_t PackValidity4(std::uint8_t first,std::uint8_t second,std::uint8_t third,std::uint8_t fourth) {
	
	return static_cast<std::uint32_t>(first) |
		(static_cast<std::uint32_t>(second) << 8u) |
		(static_cast<std::uint32_t>(third) << 16u) |
		(static_cast<std::uint32_t>(fourth) << 24u);
}

bool IsActiveBatchWorkItem(std::uint32_t batchBase,std::uint32_t localBatchIndex,std::uint32_t workItemCount) {
	return localBatchIndex < BATCH_CAPACITY && batchBase <= workItemCount && localBatchIndex < workItemCount - batchBase;
}

} // namespace FDW_SD::Baseline

namespace FDW_SD::Features {

std::uint64_t ComputePixelTensorOffset(std::uint32_t batchIndex,std::uint32_t channel,std::uint32_t localX,std::uint32_t localY) {
	
	return ((static_cast<std::uint64_t>(batchIndex) * PIXEL_CHANNEL_COUNT + channel) * PATCH_HEIGHT + localY) * PATCH_WIDTH + localX;
}

std::uint64_t ComputeLightTensorOffset(std::uint32_t batchIndex, std::uint32_t channel) {
	return static_cast<std::uint64_t>(batchIndex) * LIGHT_CHANNEL_COUNT + channel;
}

} // namespace FDW_SD::Features

namespace FDW_SD::Output {

std::uint64_t GetGeneration(const NSOutputHeader& header) {
	return static_cast<std::uint64_t>(header.GenerationLow) | (static_cast<std::uint64_t>(header.GenerationHigh) << 32u);
}

std::uint64_t GetGeneration(const NSOutputFallbackInput& fallback) {
	return static_cast<std::uint64_t>(fallback.GenerationLow) | (static_cast<std::uint64_t>(fallback.GenerationHigh) << 32u);
}

std::uint32_t ComputeVisibilityByteCount(std::uint32_t valueCount) {
	return valueCount * VISIBILITY_BYTES_PER_VALUE;
}

std::uint32_t ComputeValidityByteCount(std::uint32_t valueCount) {
	return valueCount * VALIDITY_BYTES_PER_VALUE;
}

NSOutputHeader MakeCompleteOutputHeader(std::uint64_t generation,std::uint32_t workItemCount,NSOutputProducer producer) {
	
	NSOutputHeader header{};
	header.GenerationLow = static_cast<std::uint32_t>(generation);
	header.GenerationHigh = static_cast<std::uint32_t>(generation >> 32u);
	header.Producer = static_cast<std::uint32_t>(producer);
	header.Completion = static_cast<std::uint32_t>(NSOutputCompletion::Complete);
	header.WorkItemCount = workItemCount;
	header.OutputValueCount = workItemCount * VALUES_PER_WORK_ITEM;
	header.VisibilityByteCount = ComputeVisibilityByteCount(header.OutputValueCount);
	header.ValidityByteCount = ComputeValidityByteCount(header.OutputValueCount);
	
	return header;
}

bool IsSupportedProducer(std::uint32_t producer) {
	return producer >= static_cast<std::uint32_t>(NSOutputProducer::DeterministicBaseline) && producer <= static_cast<std::uint32_t>(NSOutputProducer::Neural);
}

bool IsWorkForViewport(const FDW_SD::Work::NSWorkDispatchHeader& work,std::uint32_t viewportWidth,std::uint32_t viewportHeight) {
	const auto patchCountX = FDW_SD::Work::ComputePatchCountX(viewportWidth);
	const auto patchCountY = FDW_SD::Work::ComputePatchCountY(viewportHeight);
	
	return viewportWidth != 0u && viewportHeight != 0u &&
		work.ScreenWidth == viewportWidth &&
		work.ScreenHeight == viewportHeight &&
		work.PatchCountX == patchCountX &&
		work.PatchCountY == patchCountY &&
		static_cast<std::uint64_t>(patchCountX) * patchCountY <= FDW_SD::Work::MAX_WORK_ITEMS;
}

bool IsCompleteOutputHeader(const NSOutputHeader& output, const FDW_SD::Work::NSWorkDispatchHeader& work, std::uint64_t expectedGeneration) {
	
	return expectedGeneration != 0u &&
		output.LayoutVersion == OUTPUT_LAYOUT_VERSION &&
		GetGeneration(output) == expectedGeneration &&
		work.LayoutVersion == FDW_SD::Work::DATA_LAYOUT_VERSION &&
		FDW_SD::Work::GetGeneration(work) == expectedGeneration &&
		work.PatchWidth == FDW_SD::Work::PATCH_WIDTH &&
		work.PatchHeight == FDW_SD::Work::PATCH_HEIGHT &&
		work.WorkItemCount <= FDW_SD::Work::MAX_WORK_ITEMS &&
		output.VisibilityEncoding == static_cast<std::uint32_t>(NSVisibilityEncoding::R16Float) &&
		output.ValidityEncoding == static_cast<std::uint32_t>(NSValidityEncoding::R8Uint) &&
		IsSupportedProducer(output.Producer) &&
		output.Completion == static_cast<std::uint32_t>(NSOutputCompletion::Complete) &&
		output.FallbackPolicy == static_cast<std::uint32_t>(NSOutputFallbackPolicy::SameGenerationHardAtlasElseFullyLit) &&
		output.WorkItemCount == work.WorkItemCount &&
		output.WorkItemCount <= FDW_SD::Work::MAX_WORK_ITEMS &&
		output.OutputValueCount == output.WorkItemCount * VALUES_PER_WORK_ITEM &&
		output.OutputValueCount == work.OutputValueCount &&
		output.ValuesPerWorkItem == VALUES_PER_WORK_ITEM &&
		output.VisibilityByteCount == ComputeVisibilityByteCount(output.OutputValueCount) &&
		output.ValidityByteCount == ComputeValidityByteCount(output.OutputValueCount) &&
		output.LookupKind == static_cast<std::uint32_t>(NSOutputLookupKind::DirectPatchRangeBinarySearch) &&
		output.MaxLookupProbes == MAX_LOOKUP_PROBES &&
		output.Padding0 == 0u;
}

bool IsCompleteOutputForViewport(const NSOutputHeader& output,const FDW_SD::Work::NSWorkDispatchHeader& work,std::uint64_t expectedGeneration,std::uint32_t viewportWidth,std::uint32_t viewportHeight) {
	return IsWorkForViewport(work, viewportWidth, viewportHeight) && IsCompleteOutputHeader(output, work, expectedGeneration);
}

std::uint32_t ComputeStoredPatchItemCount(const FDW_SD::Work::NSWorkPatchRecord& patch, std::uint32_t workItemCount) {
	
	const auto start = std::min(patch.WorkItemOffset, workItemCount);
	const auto remaining = workItemCount - start;
	return std::min(patch.UniquePairCount, remaining);
}

} // namespace FDW_SD::Output

namespace FDW_SD::Spatial {

std::uint32_t DivideRoundUp(std::uint32_t value, std::uint32_t divisor) {
	return divisor == 0u ? 0u : value / divisor + (value % divisor != 0u ? 1u : 0u);
}

std::uint32_t RegionCountX(std::uint32_t width) { return DivideRoundUp(width, OUTPUT_WIDTH); }

std::uint32_t RegionCountY(std::uint32_t height) { return DivideRoundUp(height, OUTPUT_HEIGHT); }

std::uint32_t ScreenRegionIndex(std::uint32_t regionX, std::uint32_t regionY, std::uint32_t countX) {
	return regionX + regionY * countX;
}

std::uint32_t LowIndex(std::uint32_t x, std::uint32_t y) { return x + y * LOW_WIDTH; }

std::uint32_t OutputIndex(std::uint32_t x, std::uint32_t y) { return x + y * OUTPUT_WIDTH; }

std::uint32_t LowInputOffset(std::uint32_t work) { return work * LOW_VALUE_COUNT; }

std::uint32_t OutputOffset(std::uint32_t work) { return work * OUTPUT_VALUE_COUNT; }

bool IsComplete(const Header& header, std::uint64_t generation) {
	
	return generation != 0u && header.LayoutVersion == LAYOUT_VERSION &&
		(static_cast<std::uint64_t>(header.GenerationHigh) << 32u | header.GenerationLow) == generation &&
		header.ScreenWidth != 0u && header.ScreenHeight != 0u &&
		header.RegionWidth == OUTPUT_WIDTH && header.RegionHeight == OUTPUT_HEIGHT &&
		header.LowWidth == LOW_WIDTH && header.LowHeight == LOW_HEIGHT &&
		header.RegionCountX == RegionCountX(header.ScreenWidth) &&
		header.RegionCountY == RegionCountY(header.ScreenHeight) &&
		header.PixelChannelCount == PIXEL_CHANNEL_COUNT &&
		header.LightChannelCount == LIGHT_CHANNEL_COUNT &&
		header.ValuesPerWorkItem == OUTPUT_VALUE_COUNT && header.Completion == 1u;
}

} // namespace FDW_SD::Spatial

namespace FDW_SD::Work {

std::uint32_t DivideRoundUp(std::uint32_t value, std::uint32_t divisor) {
	return divisor == 0u ? 0u : value / divisor + (value % divisor != 0u ? 1u : 0u);
}

std::uint32_t ComputePatchCountX(std::uint32_t screenWidth) {
	return DivideRoundUp(screenWidth, PATCH_WIDTH);
}

std::uint32_t ComputePatchCountY(std::uint32_t screenHeight) {
	return DivideRoundUp(screenHeight, PATCH_HEIGHT);
}

std::uint32_t ComputeScreenTileIndex(std::uint32_t patchX,std::uint32_t patchY,std::uint32_t patchCountX) {
	return patchX + patchY * patchCountX;
}

std::uint32_t ComputeLocalPixelIndex(std::uint32_t localX,std::uint32_t localY) {
	return localX + localY * PATCH_WIDTH;
}

std::uint32_t ComputeOutputOffset(std::uint32_t workItemIndex) {
	return workItemIndex * VALUES_PER_WORK_ITEM;
}

std::uint32_t ComputeCoarseTileCoordinate(std::uint32_t pixel,std::uint32_t extent,std::uint32_t coarseTileCount) {
	
	if (extent == 0u || coarseTileCount == 0u) return 0u;
	
	const auto clampedPixel = std::min(pixel, extent - 1u);
	
	return std::min(static_cast<std::uint32_t>(static_cast<std::uint64_t>(clampedPixel) * coarseTileCount / extent),coarseTileCount - 1u);
}

NSWorkPatchCoarseBounds ComputePatchCoarseBounds(std::uint32_t screenTileIndex,std::uint32_t screenWidth,std::uint32_t screenHeight,std::uint32_t coarseGridWidth,std::uint32_t coarseGridHeight) {
	const auto patchCountX = ComputePatchCountX(screenWidth);
	if (patchCountX == 0u || screenHeight == 0u) return {};
	
	const auto patchX = screenTileIndex % patchCountX;
	const auto patchY = screenTileIndex / patchCountX;
	
	const auto minPixelX = patchX * PATCH_WIDTH;
	const auto minPixelY = patchY * PATCH_HEIGHT;
	const auto maxPixelX = std::min(minPixelX + PATCH_WIDTH - 1u, screenWidth - 1u);
	const auto maxPixelY = std::min(minPixelY + PATCH_HEIGHT - 1u, screenHeight - 1u);
	
	return {
		ComputeCoarseTileCoordinate(minPixelX, screenWidth, coarseGridWidth),
		ComputeCoarseTileCoordinate(minPixelY, screenHeight, coarseGridHeight),
		ComputeCoarseTileCoordinate(maxPixelX, screenWidth, coarseGridWidth),
		ComputeCoarseTileCoordinate(maxPixelY, screenHeight, coarseGridHeight)
	};
}

std::uint64_t PackLookupKey(NSWorkLookupKey key) {
	return (static_cast<std::uint64_t>(key.ScreenTileIndex) << 32u) | static_cast<std::uint64_t>(key.LightIndex);
}

bool IsLookupKeyLess(NSWorkLookupKey left, NSWorkLookupKey right) {
	return left.ScreenTileIndex < right.ScreenTileIndex || (left.ScreenTileIndex == right.ScreenTileIndex && left.LightIndex < right.LightIndex);
}

bool IsLookupKeyEqual(NSWorkLookupKey left, NSWorkLookupKey right) {
	return left.ScreenTileIndex == right.ScreenTileIndex && left.LightIndex == right.LightIndex;
}

NSWorkCapacityResult ClassifyWorkCapacity(std::uint64_t requiredWorkItems) {
	const auto publishedRequired = static_cast<std::uint32_t>(std::min( requiredWorkItems, static_cast<std::uint64_t>(std::numeric_limits<std::uint32_t>::max())));
	const auto written = static_cast<std::uint32_t>(std::min( requiredWorkItems, static_cast<std::uint64_t>(MAX_WORK_ITEMS)));
	const auto dropped = requiredWorkItems - written;
	
	std::uint32_t flags = 0u;
	if (requiredWorkItems > MAX_WORK_ITEMS) {
		flags |= static_cast<std::uint32_t>(NSWorkOverflowFlags::WorkItemCapacity);
	}
	
	if (requiredWorkItems > std::numeric_limits<std::uint32_t>::max() ||
		dropped > std::numeric_limits<std::uint32_t>::max()) {
		flags |= static_cast<std::uint32_t>(NSWorkOverflowFlags::RequiredCountSaturated);
	}
	
	return {publishedRequired,written,static_cast<std::uint32_t>(std::min(dropped, static_cast<std::uint64_t>(std::numeric_limits<std::uint32_t>::max()))), written * VALUES_PER_WORK_ITEM, flags };
}

std::uint64_t GetGeneration(const NSWorkDispatchHeader& header) {
	return static_cast<std::uint64_t>(header.GenerationLow) | (static_cast<std::uint64_t>(header.GenerationHigh) << 32u);
}

bool IsDatasetPublishable(const NSWorkDispatchHeader& header) {
	
	return header.LayoutVersion == DATA_LAYOUT_VERSION &&
		GetGeneration(header) != 0u &&
		header.ScreenWidth != 0u &&
		header.ScreenHeight != 0u &&
		header.PatchWidth == PATCH_WIDTH &&
		header.PatchHeight == PATCH_HEIGHT &&
		header.PatchCountX == ComputePatchCountX(header.ScreenWidth) &&
		header.PatchCountY == ComputePatchCountY(header.ScreenHeight) &&
		header.WorkItemCapacity == MAX_WORK_ITEMS &&
		header.OutputValueCapacity == MAX_OUTPUT_VALUES &&
		header.LookupSlotCapacity == LOOKUP_SLOT_CAPACITY &&
		header.WorkItemCount == header.RequiredWorkItemCount &&
		header.WorkItemCount <= header.WorkItemCapacity &&
		header.OutputValueCount == header.WorkItemCount * VALUES_PER_WORK_ITEM &&
		header.DroppedWorkItemCount == 0u &&
		header.RuntimeFallbackCount == 0u &&
		header.OverflowFlags == 0u &&
		header.OverflowPolicy == static_cast<std::uint32_t>(NSWorkOverflowPolicy::SameGenerationHardAtlasElseFullyLitAndRejectDataset);
}

} // namespace FDW_SD::Work

