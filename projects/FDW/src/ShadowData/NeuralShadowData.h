#pragma once

#include <ShadowData/ShadowLightParameters.h>



namespace FDW_SD::Work {

inline constexpr std::uint32_t DATA_LAYOUT_VERSION = 1u;
inline constexpr std::uint32_t PATCH_WIDTH = 16u;
inline constexpr std::uint32_t PATCH_HEIGHT = 16u;
inline constexpr std::uint32_t VALUES_PER_WORK_ITEM = PATCH_WIDTH * PATCH_HEIGHT;
inline constexpr std::uint32_t MAX_WORK_ITEMS = 262'144u;
inline constexpr std::uint32_t MAX_OUTPUT_VALUES = MAX_WORK_ITEMS * VALUES_PER_WORK_ITEM;
inline constexpr std::uint32_t LOOKUP_SLOT_CAPACITY = 524'288u;
inline constexpr std::uint32_t INVALID_INDEX = 0xffff'ffffu;

enum class NSWorkOverflowFlags : std::uint32_t {
	None = 0u,
	WorkItemCapacity = 1u << 0u,
	UpstreamTileList = 1u << 1u,
	RequiredCountSaturated = 1u << 2u,
	InvalidShadowMetadata = 1u << 3u,
	DuplicateOutputPair = 1u << 4u
};

enum class NSWorkOverflowPolicy : std::uint32_t {
	SameGenerationHardAtlasElseFullyLitAndRejectDataset = 1u
};

struct NSWorkItem {
	std::uint32_t ScreenTileIndex = 0u;
	std::uint32_t LightIndex = 0u;
	std::uint32_t ShadowMetaIndex = 0u;
	std::uint32_t OutputOffset = 0u;
};

struct NSWorkLookupKey {
	std::uint32_t ScreenTileIndex = INVALID_INDEX;
	std::uint32_t LightIndex = INVALID_INDEX;
};

struct NSWorkDispatchHeader {
	std::uint32_t LayoutVersion = DATA_LAYOUT_VERSION;
	std::uint32_t GenerationLow = 0u;
	std::uint32_t GenerationHigh = 0u;
	std::uint32_t ScreenWidth = 0u;
	std::uint32_t ScreenHeight = 0u;
	std::uint32_t PatchWidth = PATCH_WIDTH;
	std::uint32_t PatchHeight = PATCH_HEIGHT;
	std::uint32_t PatchCountX = 0u;
	std::uint32_t PatchCountY = 0u;
	std::uint32_t WorkItemCount = 0u;
	std::uint32_t RequiredWorkItemCount = 0u;
	std::uint32_t WorkItemCapacity = MAX_WORK_ITEMS;
	std::uint32_t OutputValueCount = 0u;
	std::uint32_t OutputValueCapacity = MAX_OUTPUT_VALUES;
	std::uint32_t LookupSlotCapacity = LOOKUP_SLOT_CAPACITY;
	std::uint32_t DroppedWorkItemCount = 0u;
	std::uint32_t DuplicateCandidateCount = 0u;
	std::uint32_t RuntimeFallbackCount = 0u;
	std::uint32_t OverflowFlags = 0u;
	std::uint32_t OverflowPolicy = static_cast<std::uint32_t>(NSWorkOverflowPolicy::SameGenerationHardAtlasElseFullyLitAndRejectDataset);
};

struct NSWorkPatchRecord {
	std::uint32_t UniquePairCount = 0u;
	std::uint32_t DuplicateCandidateCount = 0u;
	std::uint32_t WorkItemOffset = 0u;
	std::uint32_t Padding0 = 0u;
};

struct NSWorkGenerationParams {
	std::uint32_t ScreenWidth = 0u;
	std::uint32_t ScreenHeight = 0u;
	std::uint32_t PatchCountX = 0u;
	std::uint32_t PatchCountY = 0u;
	std::uint32_t LightCount = 0u;
	std::uint32_t ShadowMetaCount = 0u;
	std::uint32_t ActiveShadowWork = 0u;
	std::uint32_t GenerationLow = 0u;
	std::uint32_t GenerationHigh = 0u;
	std::uint32_t CoarseGridWidth = 16u;
	std::uint32_t CoarseGridHeight = 9u;
	std::uint32_t CoarseTileCount = 144u;
};

struct NSWorkPatchCoarseBounds {
	std::uint32_t MinTileX = 0u;
	std::uint32_t MinTileY = 0u;
	std::uint32_t MaxTileX = 0u;
	std::uint32_t MaxTileY = 0u;
};

struct NSWorkCapacityResult {
	std::uint32_t RequiredWorkItemCount = 0u;
	std::uint32_t WorkItemCount = 0u;
	std::uint32_t DroppedWorkItemCount = 0u;
	std::uint32_t OutputValueCount = 0u;
	std::uint32_t OverflowFlags = 0u;
};

std::uint32_t DivideRoundUp(std::uint32_t value, std::uint32_t divisor);

std::uint32_t ComputePatchCountX(std::uint32_t screenWidth);
std::uint32_t ComputePatchCountY(std::uint32_t screenHeight);

std::uint32_t ComputeScreenTileIndex(std::uint32_t patchX, std::uint32_t patchY, std::uint32_t patchCountX);
std::uint32_t ComputeLocalPixelIndex(std::uint32_t localX, std::uint32_t localY);

std::uint32_t ComputeOutputOffset(std::uint32_t workItemIndex);

std::uint32_t ComputeCoarseTileCoordinate(std::uint32_t pixel, std::uint32_t extent, std::uint32_t coarseTileCount);

NSWorkPatchCoarseBounds ComputePatchCoarseBounds( std::uint32_t screenTileIndex, std::uint32_t screenWidth, std::uint32_t screenHeight, std::uint32_t coarseGridWidth, std::uint32_t coarseGridHeight);

std::uint64_t PackLookupKey(NSWorkLookupKey key);

bool IsLookupKeyLess(NSWorkLookupKey left, NSWorkLookupKey right);

bool IsLookupKeyEqual(NSWorkLookupKey left, NSWorkLookupKey right);

NSWorkCapacityResult ClassifyWorkCapacity(std::uint64_t requiredWorkItems);

std::uint64_t GetGeneration(const NSWorkDispatchHeader& header);

bool IsDatasetPublishable(const NSWorkDispatchHeader& header);

}



namespace FDW_SD::Features {

inline constexpr std::uint32_t PATCH_WIDTH = 16u;
inline constexpr std::uint32_t PATCH_HEIGHT = 16u;
inline constexpr std::uint32_t PIXELS_PER_PATCH = PATCH_WIDTH * PATCH_HEIGHT;
inline constexpr std::uint32_t PIXEL_CHANNEL_COUNT = 10u;
inline constexpr std::uint32_t FEATURE_LAYOUT_VERSION = 2u;
inline constexpr std::uint32_t LIGHT_CHANNEL_COUNT = 14u;
inline constexpr float MAX_DIRECTIONAL_ANGULAR_RADIUS = 0.5f;
inline constexpr float MAX_SHADOW_SOFTNESS = 4.0f;
inline constexpr float NORMALIZATION_EPSILON = 1.0e-6f;
inline constexpr float CPU_HLSL_TOLERANCE = 1.0e-5f;
inline constexpr float FP16_STORAGE_TOLERANCE = 1.0e-3f;

enum class NSFeatureTensorLayout : std::uint32_t {
	NCHW = 1u
};

enum class NSFeatureStorageType : std::uint32_t {
	IEEEFloat16 = 1u
};

enum class NSPixelFeatureChannel : std::uint32_t {
	HardVisibility = 0u,
	ReceiverLogDepth = 1u,
	ReceiverNormalX = 2u,
	ReceiverNormalY = 3u,
	ReceiverNormalZ = 4u,
	DirectionToLightX = 5u,
	DirectionToLightY = 6u,
	DirectionToLightZ = 7u,
	NormalizedLightDistance = 8u,
	ReceiverValid = 9u
};

enum class NSLightFeatureChannel : std::uint32_t {
	TypePoint = 0u,
	TypeSpot = 1u,
	TypeDirectional = 2u,
	TypeRectangle = 3u,
	AttenuationRadiusOverCameraFar = 4u,
	PointOrSpotSourceRadiusOverAttenuation = 5u,
	RectangleWidthOverAttenuation = 6u,
	RectangleHeightOverAttenuation = 7u,
	RectangleRotationQuaternionX = 8u,
	RectangleRotationQuaternionY = 9u,
	RectangleRotationQuaternionZ = 10u,
	RectangleRotationQuaternionW = 11u,
	DirectionalAngularRadius = 12u,
	ShadowSoftness = 13u
};

inline constexpr std::int32_t LIGHT_TYPE_POINT = 0;
inline constexpr std::int32_t LIGHT_TYPE_SPOT = 1;
inline constexpr std::int32_t LIGHT_TYPE_DIRECTIONAL = 2;
inline constexpr std::int32_t LIGHT_TYPE_RECTANGLE = 3;

struct NSFeatureFloat3 {
	float X = 0.0f;
	float Y = 0.0f;
	float Z = 0.0f;
};

struct NSFeatureFloat4 {
	float X = 0.0f;
	float Y = 0.0f;
	float Z = 0.0f;
	float W = 0.0f;
};

struct NSPixelFeatureInput {
	float HardVisibility = 1.0f;
	float ReceiverLinearDepth = 0.0f;
	float CameraNear = 0.0f;
	float CameraFar = 0.0f;
	NSFeatureFloat3 ReceiverNormal{};
	NSFeatureFloat3 ReceiverWorldPosition{};
	std::uint32_t ReceiverValid = 0u;
};

struct NSLightFeatureInput {
	std::int32_t LightType = LIGHT_TYPE_POINT;
	NSFeatureFloat3 Position{};
	float AttenuationRadius = 0.0f;
	float SourceRadius = 0.0f;
	NSFeatureFloat3 Direction{};
	float RectangleWidth = 0.0f;
	float RectangleHeight = 0.0f;
	NSFeatureFloat3 Rotation{};
	float DirectionalAngularRadius = 0.0f;
	float ShadowSoftness = 1.0f;
};

using NSPixelFeatures = std::array<float, PIXEL_CHANNEL_COUNT>;
using NSLightFeatures = std::array<float, LIGHT_CHANNEL_COUNT>;

std::uint64_t ComputePixelTensorOffset(
	std::uint32_t batchIndex,
	std::uint32_t channel,
	std::uint32_t localX,
	std::uint32_t localY);

std::uint64_t ComputeLightTensorOffset(std::uint32_t batchIndex, std::uint32_t channel);

}



namespace FDW_SD::Output {

inline constexpr std::uint32_t OUTPUT_LAYOUT_VERSION = 1u;
inline constexpr std::uint32_t VALUES_PER_WORK_ITEM = FDW_SD::Work::VALUES_PER_WORK_ITEM;
inline constexpr std::uint32_t MAX_OUTPUT_VALUES = FDW_SD::Work::MAX_OUTPUT_VALUES;
inline constexpr std::uint32_t VISIBILITY_BYTES_PER_VALUE = 2u;
inline constexpr std::uint32_t VALIDITY_BYTES_PER_VALUE = 1u;
inline constexpr std::uint32_t MAX_LOOKUP_PROBES = 19u;
inline constexpr std::uint32_t INVALID_INDEX = FDW_SD::Work::INVALID_INDEX;

enum class NSVisibilityEncoding : std::uint32_t {
	R16Float = 1u
};

enum class NSValidityEncoding : std::uint32_t {
	R8Uint = 1u
};

enum class NSOutputProducer : std::uint32_t {
	DeterministicBaseline = 1u,
	RayTracedReference = 2u,
	Neural = 3u
};

enum class NSOutputCompletion : std::uint32_t {
	Incomplete = 0u,
	Complete = 1u
};

enum class NSOutputFallbackPolicy : std::uint32_t {
	SameGenerationHardAtlasElseFullyLit = 1u
};

enum class NSOutputLookupKind : std::uint32_t {
	DirectPatchRangeBinarySearch = 1u
};

enum class NSOutputLookupStatus : std::uint32_t {
	Found = 0u,
	MissingPair = 1u,
	GenerationMismatch = 2u,
	IncompleteOutput = 3u,
	InvalidLayout = 4u,
	InvalidPatch = 5u,
	InvalidWorkRange = 6u
};

enum class NSOutputVisibilitySource : std::uint32_t {
	CompactOutput = 0u,
	SameGenerationHardAtlas = 1u,
	FullyLit = 2u
};

enum class NSOutputValidityReason : std::uint32_t {
	Valid = 0u,
	PaddedOrNoReceiver = 1u,
	InvalidLightIdentity = 2u,
	OutsideLightDomain = 3u,
	MissingAccelerationStructure = 4u,
	UnsupportedLightType = 5u,
	UnsupportedEmitterExtent = 6u,
	UnsupportedGeometryCoverage = 7u,
	InvalidRayInterval = 8u,
	CapacityOverflow = 9u,
	GenerationMismatch = 10u,
	Incomplete = 11u
};

struct NSOutputHeader {
	std::uint32_t LayoutVersion = OUTPUT_LAYOUT_VERSION;
	std::uint32_t GenerationLow = 0u;
	std::uint32_t GenerationHigh = 0u;
	std::uint32_t VisibilityEncoding = static_cast<std::uint32_t>(NSVisibilityEncoding::R16Float);
	std::uint32_t ValidityEncoding = static_cast<std::uint32_t>(NSValidityEncoding::R8Uint);
	std::uint32_t Producer = static_cast<std::uint32_t>(NSOutputProducer::DeterministicBaseline);
	std::uint32_t Completion = static_cast<std::uint32_t>(NSOutputCompletion::Incomplete);
	std::uint32_t FallbackPolicy = static_cast<std::uint32_t>(NSOutputFallbackPolicy::SameGenerationHardAtlasElseFullyLit);
	std::uint32_t WorkItemCount = 0u;
	std::uint32_t OutputValueCount = 0u;
	std::uint32_t ValuesPerWorkItem = VALUES_PER_WORK_ITEM;
	std::uint32_t VisibilityByteCount = 0u;
	std::uint32_t ValidityByteCount = 0u;
	std::uint32_t LookupKind = static_cast<std::uint32_t>(NSOutputLookupKind::DirectPatchRangeBinarySearch);
	std::uint32_t MaxLookupProbes = MAX_LOOKUP_PROBES;
	std::uint32_t Padding0 = 0u;
};

struct NSOutputLookupResult {
	std::uint32_t WorkItemIndex = INVALID_INDEX;
	std::uint32_t OutputOffset = INVALID_INDEX;
	std::uint32_t ProbeCount = 0u;
	std::uint32_t Status = static_cast<std::uint32_t>(NSOutputLookupStatus::MissingPair);
};

struct NSOutputFallbackInput {
	std::uint32_t Available = 0u;
	std::uint32_t GenerationLow = 0u;
	std::uint32_t GenerationHigh = 0u;
	float Visibility = 1.0f;
};

struct NSOutputResolveResult {
	float Visibility = 1.0f;
	std::uint32_t Source = static_cast<std::uint32_t>(NSOutputVisibilitySource::FullyLit);
	std::uint32_t ValidityReason = static_cast<std::uint32_t>(NSOutputValidityReason::Incomplete);
	std::uint32_t WorkItemIndex = INVALID_INDEX;
};

std::uint64_t GetGeneration(const NSOutputHeader& header);

std::uint64_t GetGeneration(const NSOutputFallbackInput& fallback);

std::uint32_t ComputeVisibilityByteCount(std::uint32_t valueCount);

std::uint32_t ComputeValidityByteCount(std::uint32_t valueCount);

NSOutputHeader MakeCompleteOutputHeader(std::uint64_t generation, std::uint32_t workItemCount, NSOutputProducer producer);

bool IsSupportedProducer(std::uint32_t producer);

bool IsWorkForViewport(const FDW_SD::Work::NSWorkDispatchHeader& work, std::uint32_t viewportWidth, std::uint32_t viewportHeight);

bool IsCompleteOutputHeader( const NSOutputHeader& output, const FDW_SD::Work::NSWorkDispatchHeader& work, std::uint64_t expectedGeneration);

bool IsCompleteOutputForViewport( const NSOutputHeader& output, const FDW_SD::Work::NSWorkDispatchHeader& work, std::uint64_t expectedGeneration, std::uint32_t viewportWidth, std::uint32_t viewportHeight);

std::uint32_t ComputeStoredPatchItemCount( const FDW_SD::Work::NSWorkPatchRecord& patch, std::uint32_t workItemCount);


}



namespace FDW_SD::Spatial {

inline constexpr std::uint32_t LAYOUT_VERSION = 3u;
inline constexpr std::uint32_t LOW_WIDTH = 16u;
inline constexpr std::uint32_t LOW_HEIGHT = 16u;
inline constexpr std::uint32_t OUTPUT_WIDTH = 64u;
inline constexpr std::uint32_t OUTPUT_HEIGHT = 64u;
inline constexpr std::uint32_t UPSCALE_FACTOR = 4u;
inline constexpr std::uint32_t LOW_VALUE_COUNT = LOW_WIDTH * LOW_HEIGHT;
inline constexpr std::uint32_t OUTPUT_VALUE_COUNT = OUTPUT_WIDTH * OUTPUT_HEIGHT;
inline constexpr std::uint32_t PIXEL_CHANNEL_COUNT = 16u;
inline constexpr std::uint32_t LIGHT_CHANNEL_COUNT = 14u;
inline constexpr std::uint32_t RECEIVER_CHANNEL_COUNT = 9u;
inline constexpr std::uint32_t INVALID_INDEX = 0xffff'ffffu;
inline constexpr std::uint32_t INPUT_VALID_HARD_BIT = 1u << 0u;
inline constexpr std::uint32_t INPUT_VALID_BLOCKER_BIT = 1u << 1u;
inline constexpr float LOCAL_EFFECTIVE_RADIUS_NORMALIZATION = 40.0f;
inline constexpr float DIRECTIONAL_EFFECTIVE_RADIUS_NORMALIZATION = 0.5f;

enum class PixelChannel : std::uint32_t {
	HardNearest = 0u,
	HardBilinear = 1u,
	HardCoverage = 2u,
	HardSignedBoundaryDistance = 3u,
	NormalizedFirstHitDistance = 4u,
	HardValid = 5u,
	BlockerValid = 6u,
	ReceiverLogDepth = 7u,
	ReceiverNormalX = 8u,
	ReceiverNormalY = 9u,
	ReceiverNormalZ = 10u,
	DirectionToLightX = 11u,
	DirectionToLightY = 12u,
	DirectionToLightZ = 13u,
	NormalizedLightDistance = 14u,
	ReceiverValid = 15u
};

enum class LightChannel : std::uint32_t {
	TypePoint = 0u,
	TypeSpot = 1u,
	TypeDirectional = 2u,
	TypeRectangle = 3u,
	AttenuationOverCameraFar = 4u,
	EffectiveLocalRadiusOver40 = 5u,
	EffectiveRectangleWidthOver4 = 6u,
	EffectiveRectangleHeightOver4 = 7u,
	RotationQuaternionX = 8u,
	RotationQuaternionY = 9u,
	RotationQuaternionZ = 10u,
	RotationQuaternionW = 11u,
	EffectiveDirectionalRadiusOverPoint5 = 12u,
	ShadowSoftnessOver4 = 13u
};

struct WorkItem {
	std::uint32_t ScreenRegionIndex = 0u;
	std::uint32_t LightIndex = 0u;
	std::uint32_t ShadowMetaIndex = INVALID_INDEX;
	std::uint32_t LowInputOffset = 0u;
	std::uint32_t OutputOffset = 0u;
	std::uint32_t Padding0 = 0u;
	std::uint32_t Padding1 = 0u;
	std::uint32_t Padding2 = 0u;
};

struct Header {
	std::uint32_t LayoutVersion = LAYOUT_VERSION;
	std::uint32_t GenerationLow = 0u;
	std::uint32_t GenerationHigh = 0u;
	std::uint32_t ScreenWidth = 0u;
	std::uint32_t ScreenHeight = 0u;
	std::uint32_t RegionWidth = OUTPUT_WIDTH;
	std::uint32_t RegionHeight = OUTPUT_HEIGHT;
	std::uint32_t LowWidth = LOW_WIDTH;
	std::uint32_t LowHeight = LOW_HEIGHT;
	std::uint32_t RegionCountX = 0u;
	std::uint32_t RegionCountY = 0u;
	std::uint32_t WorkItemCount = 0u;
	std::uint32_t PixelChannelCount = PIXEL_CHANNEL_COUNT;
	std::uint32_t LightChannelCount = LIGHT_CHANNEL_COUNT;
	std::uint32_t ValuesPerWorkItem = OUTPUT_VALUE_COUNT;
	std::uint32_t Completion = 0u;
};

std::uint32_t DivideRoundUp(std::uint32_t value, std::uint32_t divisor);

std::uint32_t RegionCountX(std::uint32_t width);
std::uint32_t RegionCountY(std::uint32_t height);
std::uint32_t ScreenRegionIndex(std::uint32_t regionX, std::uint32_t regionY, std::uint32_t countX);
std::uint32_t LowIndex(std::uint32_t x, std::uint32_t y);
std::uint32_t OutputIndex(std::uint32_t x, std::uint32_t y);
std::uint32_t LowInputOffset(std::uint32_t work);
std::uint32_t OutputOffset(std::uint32_t work);

bool IsComplete(const Header& header, std::uint64_t generation);

}

namespace FDW_SD::RuntimeOutput {

inline constexpr std::uint32_t OUTPUT_LAYOUT_VERSION = 2u;
inline constexpr std::uint32_t VISIBILITY_ENCODING_FLOAT32 = 1u;
inline constexpr std::uint32_t VALIDITY_ENCODING_UINT32 = 1u;
inline constexpr std::uint32_t LOOKUP_SORTED_REGION_LIGHT = 1u;
inline constexpr std::uint32_t COMPLETION_INCOMPLETE = 0u;
inline constexpr std::uint32_t COMPLETION_COMPLETE = 1u;
inline constexpr std::uint32_t VALIDITY_VALID = 0u;
inline constexpr std::uint32_t VALIDITY_NO_RECEIVER_OR_OUTSIDE_DOMAIN = 1u;

bool IsOutputValueValid(std::uint32_t validity);

struct OutputHeader {
    std::uint32_t LayoutVersion = OUTPUT_LAYOUT_VERSION;
    std::uint32_t SpatialLayoutVersion = FDW_SD::Spatial::LAYOUT_VERSION;
    std::uint32_t GenerationLow = 0u;
    std::uint32_t GenerationHigh = 0u;
    std::uint32_t ScreenWidth = 0u;
    std::uint32_t ScreenHeight = 0u;
    std::uint32_t RegionCountX = 0u;
    std::uint32_t RegionCountY = 0u;
    std::uint32_t WorkItemCount = 0u;
    std::uint32_t ValuesPerWorkItem = FDW_SD::Spatial::OUTPUT_VALUE_COUNT;
    std::uint32_t VisibilityEncoding = VISIBILITY_ENCODING_FLOAT32;
    std::uint32_t ValidityEncoding = VALIDITY_ENCODING_UINT32;
    std::uint32_t LookupKind = LOOKUP_SORTED_REGION_LIGHT;
    std::uint32_t Completion = COMPLETION_INCOMPLETE;
    std::uint32_t Padding0 = 0u;
    std::uint32_t Padding1 = 0u;
};

std::uint64_t Generation(const OutputHeader& header);

bool IsComplete(const OutputHeader& header);

}




namespace FDW_SD::Baseline {

inline constexpr std::uint32_t BATCH_CAPACITY = 1024u;
inline constexpr std::uint32_t BATCH_COUNT = (FDW_SD::Work::MAX_WORK_ITEMS + BATCH_CAPACITY - 1u) / BATCH_CAPACITY;
inline constexpr std::uint32_t PIXEL_FEATURE_HALF_COUNT = BATCH_CAPACITY * FDW_SD::Features::PIXEL_CHANNEL_COUNT * FDW_SD::Features::PIXELS_PER_PATCH;
inline constexpr std::uint32_t LIGHT_FEATURE_HALF_COUNT = BATCH_CAPACITY * FDW_SD::Features::LIGHT_CHANNEL_COUNT;

inline constexpr std::uint32_t PIXEL_FEATURE_PACKED_WORD_COUNT = PIXEL_FEATURE_HALF_COUNT / 2u;
inline constexpr std::uint32_t LIGHT_FEATURE_PACKED_WORD_COUNT = LIGHT_FEATURE_HALF_COUNT / 2u;
inline constexpr std::uint32_t PREPARED_VALIDITY_PACKED_WORD_COUNT = BATCH_CAPACITY * FDW_SD::Features::PIXELS_PER_PATCH / 4u;
inline constexpr std::uint32_t VISIBILITY_PACKED_WORD_COUNT = FDW_SD::Output::MAX_OUTPUT_VALUES / 2u;
inline constexpr std::uint32_t VALIDITY_PACKED_WORD_COUNT = FDW_SD::Output::MAX_OUTPUT_VALUES / 4u;
inline constexpr std::uint64_t PIXEL_FEATURE_BYTE_COUNT = static_cast<std::uint64_t>(PIXEL_FEATURE_HALF_COUNT) * 2u;
inline constexpr std::uint64_t LIGHT_FEATURE_BYTE_COUNT = static_cast<std::uint64_t>(LIGHT_FEATURE_HALF_COUNT) * 2u;

enum class NSBaselinePass : std::uint32_t {
	Initialize = 0u,
	PrepareFeatures = 1u,
	ResolveNearest = 2u,
	Finalize = 3u
};

struct NSBaselineDispatchParams {
	std::uint32_t ScreenWidth = 0u;
	std::uint32_t ScreenHeight = 0u;
	std::uint32_t AtlasWidth = 0u;
	std::uint32_t AtlasHeight = 0u;
	std::uint32_t LightCount = 0u;
	std::uint32_t ShadowMetaCount = 0u;
	std::uint32_t BatchCapacity = BATCH_CAPACITY;
	std::uint32_t Padding = 0u;
	float CameraNear = 0.1f;
	float CameraFar = 10000.0f;
	std::uint32_t Padding0 = 0u;
	std::uint32_t Padding1 = 0u;
	dx::XMMATRIX InverseViewProjectionMatrix{};
};

struct NSNearestAtlasAddress {
	std::uint32_t X = 0u;
	std::uint32_t Y = 0u;
	std::uint32_t Valid = 0u;
};

NSNearestAtlasAddress ComputeNearestAtlasAddress( const LightAtlasMeta& meta, std::uint32_t screenX, std::uint32_t screenY, std::uint32_t screenWidth, std::uint32_t screenHeight, std::uint32_t atlasWidth, std::uint32_t atlasHeight);

std::uint32_t PackHalf2(std::uint16_t first, std::uint16_t second);

std::uint32_t PackValidity4( std::uint8_t first, std::uint8_t second, std::uint8_t third, std::uint8_t fourth);

bool IsActiveBatchWorkItem( std::uint32_t batchBase, std::uint32_t localBatchIndex, std::uint32_t workItemCount);


}

namespace FDW_SD::Model {

inline constexpr std::uint32_t DOWNSAMPLE = 4u;
inline constexpr std::uint32_t BASE_CHANNELS = 9u;
inline constexpr std::uint32_t DESCRIPTOR_CHANNELS = 18u;
inline constexpr std::uint32_t EMITTER_SAMPLE_COUNT = 9u;
inline constexpr std::uint32_t RECEPTIVE_FIELD = 29u;
inline constexpr std::uint32_t RECEPTIVE_FIELD_RADIUS = 14u;
inline constexpr float LIGHT_DISTANCE_NORMALIZER = 24.0f;
inline constexpr float LIGHT_RADIUS_NORMALIZER = 20.0f;

struct EmitterOffset {
    float U;
    float V;
};

inline constexpr std::array<EmitterOffset, EMITTER_SAMPLE_COUNT> EMITTER_OFFSETS{{
    {-0.6f, -0.6f},
    {0.0f, -0.6f},
    {0.6f, -0.6f},
    {-0.6f, 0.0f},
    {0.0f, 0.0f},
    {0.6f, 0.0f},
    {-0.6f, 0.6f},
    {0.0f, 0.6f},
    {0.6f, 0.6f}
}};

std::uint32_t LowExtent(std::uint32_t fullExtent) noexcept;

std::uint64_t NeuralRaysPerLight(std::uint32_t width, std::uint32_t height) noexcept;

std::uint64_t HardRaysPerLight(std::uint32_t width, std::uint32_t height) noexcept;

std::uint64_t DescriptorBytesPerLight(std::uint32_t width, std::uint32_t height) noexcept;

std::uint32_t HitChannel(std::uint32_t sample) noexcept;

std::uint32_t DistanceChannel(std::uint32_t sample) noexcept;

float SampleHardVisibility( const float* low, std::uint32_t lowWidth, std::uint32_t lowHeight, std::uint32_t x, std::uint32_t y) noexcept;

}
