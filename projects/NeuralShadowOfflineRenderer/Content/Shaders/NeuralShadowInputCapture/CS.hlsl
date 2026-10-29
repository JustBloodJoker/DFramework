#include "RS.hlsl"
#include "Structures.hlsli"
#include "NeuralShadowWorkItemData.hlsli"

StructuredBuffer<NSWorkItem> WorkItems : register(t0);
StructuredBuffer<NSWorkDispatchHeader> WorkHeader : register(t1);
StructuredBuffer<LightAtlasMeta> ShadowMetas : register(t2);
Texture2D<float> BlockerDistanceAtlas : register(t3);
Texture2D<uint> BlockerValidityAtlas : register(t4);
RWStructuredBuffer<uint> BlockerValues : register(u0);
RWStructuredBuffer<uint> InputValidity : register(u1);

struct CaptureParams
{
    uint ScreenWidth;
    uint ScreenHeight;
    uint AtlasWidth;
    uint AtlasHeight;
    uint ShadowMetaCount;
    uint WorkCapacity;
    uint LayoutVersion;
    uint Padding0;
};
ConstantBuffer<CaptureParams> Params : register(b0);

void LoadAtlasInput(NSWorkItem item, uint2 globalPixel, out float blocker, out uint flags)
{
    blocker = 1.0f;
    flags = 0u;
    if (item.ShadowMetaIndex >= Params.ShadowMetaCount) {
        return;
    }
    LightAtlasMeta meta = ShadowMetas[item.ShadowMetaIndex];
    if (meta.LightIndex != item.LightIndex || meta.AtlasWidth == 0u || meta.AtlasHeight == 0u ||
        meta.ScreenMaxU <= meta.ScreenMinU || meta.ScreenMaxV <= meta.ScreenMinV ||
        meta.AtlasOffsetX >= Params.AtlasWidth || meta.AtlasOffsetY >= Params.AtlasHeight ||
        meta.AtlasWidth > Params.AtlasWidth - meta.AtlasOffsetX ||
        meta.AtlasHeight > Params.AtlasHeight - meta.AtlasOffsetY) return;
    float2 uv = (float2(globalPixel) + 0.5f) / float2(Params.ScreenWidth, Params.ScreenHeight);
    if (uv.x < meta.ScreenMinU || uv.x >= meta.ScreenMaxU || uv.y < meta.ScreenMinV || uv.y >= meta.ScreenMaxV)
        return;
    float2 relative = saturate((uv - float2(meta.ScreenMinU, meta.ScreenMinV)) /
        float2(meta.ScreenMaxU - meta.ScreenMinU, meta.ScreenMaxV - meta.ScreenMinV));
    uint2 atlasMinimum = uint2(meta.AtlasOffsetX, meta.AtlasOffsetY);
    uint2 atlasPixel = min(
        atlasMinimum + uint2(relative * float2(meta.AtlasWidth, meta.AtlasHeight)),
        atlasMinimum + uint2(meta.AtlasWidth - 1u, meta.AtlasHeight - 1u));
    blocker = saturate(BlockerDistanceAtlas.Load(int3(atlasPixel, 0)));
    flags = BlockerValidityAtlas.Load(int3(atlasPixel, 0)) & 3u;
}

[RootSignature(RS)]
[numthreads(64, 1, 1)]
void CS(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    uint quartet = dispatchThreadId.x;
    uint workIndex = quartet / 64u;
    uint localQuartet = quartet % 64u;
    if (Params.LayoutVersion != 2u || workIndex >= Params.WorkCapacity ||
        workIndex >= WorkHeader[0].WorkItemCount) return;
    NSWorkItem item = WorkItems[workIndex];
    uint patchX = item.ScreenTileIndex % WorkHeader[0].PatchCountX;
    uint patchY = item.ScreenTileIndex / WorkHeader[0].PatchCountX;
    float values[4];
    uint flags[4];
    [unroll] for (uint lane = 0u; lane < 4u; ++lane)
    {
        uint localPixel = localQuartet * 4u + lane;
        uint2 globalPixel = uint2(patchX * 16u + localPixel % 16u, patchY * 16u + localPixel / 16u);
        values[lane] = 1.0f;
        flags[lane] = 0u;
        if (globalPixel.x < Params.ScreenWidth && globalPixel.y < Params.ScreenHeight)
            LoadAtlasInput(item, globalPixel, values[lane], flags[lane]);
    }
    uint valueWord = (workIndex * 256u + localQuartet * 4u) / 2u;
    BlockerValues[valueWord] = f32tof16(values[0]) | (f32tof16(values[1]) << 16u);
    BlockerValues[valueWord + 1u] = f32tof16(values[2]) | (f32tof16(values[3]) << 16u);
    InputValidity[workIndex * 64u + localQuartet] =
        flags[0] | (flags[1] << 8u) | (flags[2] << 16u) | (flags[3] << 24u);
}
