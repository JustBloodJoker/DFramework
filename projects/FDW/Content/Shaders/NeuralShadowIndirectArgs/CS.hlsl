#include "NeuralShadowWorkItemData.hlsli"

StructuredBuffer<NSWorkDispatchHeader> WorkHeader : register(t0);
RWStructuredBuffer<uint> DispatchArguments : register(u0);

cbuffer ArgumentConstants : register(b0)
{
    uint BatchStart;
    uint ArgumentIndex;
};

[numthreads(1, 1, 1)]
void CS(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    uint totalWork = min(WorkHeader[0].WorkItemCount, NS_WORK_MAX_ITEMS);
    uint activeCount = BatchStart >= totalWork ? 0u : min(1024u, totalWork - BatchStart);
    uint argumentOffset = ArgumentIndex * 3u;
    DispatchArguments[argumentOffset] = activeCount;
    DispatchArguments[argumentOffset + 1u] = 1u;
    DispatchArguments[argumentOffset + 2u] = 1u;
}
