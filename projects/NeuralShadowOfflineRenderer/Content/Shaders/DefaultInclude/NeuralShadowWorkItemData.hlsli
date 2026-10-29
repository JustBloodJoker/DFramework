#ifndef NEURAL_SHADOW_WORK_ITEM_DATA_HLSLI
#define NEURAL_SHADOW_WORK_ITEM_DATA_HLSLI

static const uint NS_WORK_LAYOUT_VERSION = 1u;
static const uint NS_WORK_PATCH_WIDTH = 16u;
static const uint NS_WORK_PATCH_HEIGHT = 16u;
static const uint NS_WORK_VALUES_PER_ITEM = 256u;
static const uint NS_WORK_MAX_ITEMS = 262144u;
static const uint NS_WORK_MAX_OUTPUT_VALUES = 67108864u;
static const uint NS_WORK_LOOKUP_SLOT_CAPACITY = 524288u;
static const uint NS_WORK_INVALID_INDEX = 0xffffffffu;

static const uint NS_WORK_OVERFLOW_NONE = 0u;
static const uint NS_WORK_OVERFLOW_ITEM_CAPACITY = 1u << 0u;
static const uint NS_WORK_OVERFLOW_UPSTREAM_TILE_LIST = 1u << 1u;
static const uint NS_WORK_OVERFLOW_REQUIRED_COUNT_SATURATED = 1u << 2u;
static const uint NS_WORK_OVERFLOW_INVALID_SHADOW_METADATA = 1u << 3u;
static const uint NS_WORK_OVERFLOW_DUPLICATE_OUTPUT_PAIR = 1u << 4u;

static const uint NS_WORK_OVERFLOW_POLICY = 1u;

struct NSWorkItem
{
    uint ScreenTileIndex;
    uint LightIndex;
    uint ShadowMetaIndex;
    uint OutputOffset;
};

struct NSWorkLookupKey
{
    uint ScreenTileIndex;
    uint LightIndex;
};

struct NSWorkDispatchHeader
{
    uint LayoutVersion;
    uint GenerationLow;
    uint GenerationHigh;
    uint ScreenWidth;
    uint ScreenHeight;
    uint PatchWidth;
    uint PatchHeight;
    uint PatchCountX;
    uint PatchCountY;
    uint WorkItemCount;
    uint RequiredWorkItemCount;
    uint WorkItemCapacity;
    uint OutputValueCount;
    uint OutputValueCapacity;
    uint LookupSlotCapacity;
    uint DroppedWorkItemCount;
    uint DuplicateCandidateCount;
    uint RuntimeFallbackCount;
    uint OverflowFlags;
    uint OverflowPolicy;
};

struct NSWorkPatchRecord
{
    uint UniquePairCount;
    uint DuplicateCandidateCount;
    uint WorkItemOffset;
    uint Padding0;
};

struct NSWorkGenerationParams
{
    uint ScreenWidth;
    uint ScreenHeight;
    uint PatchCountX;
    uint PatchCountY;
    uint LightCount;
    uint ShadowMetaCount;
    uint ActiveShadowWork;
    uint GenerationLow;
    uint GenerationHigh;
    uint CoarseGridWidth;
    uint CoarseGridHeight;
    uint CoarseTileCount;
};

#endif
