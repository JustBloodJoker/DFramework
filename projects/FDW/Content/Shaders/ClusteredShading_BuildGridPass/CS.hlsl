#include "Structures.hlsli"

RWStructuredBuffer<Cluster> Clusters : register(u0);
ConstantBuffer<ClusterParams> ClusterData: register(b0);

float3 ScreenToViewDepth(float2 screenCoord, float ndcZ)
{
    float2 scrDim = float2(ClusterData.ScreenWidth, ClusterData.ScreenHeight);
    float2 ndcXY = (screenCoord / scrDim) * 2.0f - 1.0f;
    float4 ndc = float4(ndcXY, ndcZ, 1.0f);
    float4 view = mul(ndc, ClusterData.InverseProjection);
    view /= view.w;
    return view.xyz;
}

[numthreads(1, 1, 1)]
void CS(uint3 groupID : SV_DispatchThreadID)
{
    uint tileIndex = groupID.x +
        (groupID.y * ClusterData.GridSize0) +
        (groupID.z * ClusterData.GridSize0 * ClusterData.GridSize1);

   float2 tileSize;
    tileSize.x = ClusterData.ScreenWidth / (float)ClusterData.GridSize0;
    tileSize.y = ClusterData.ScreenHeight / (float)ClusterData.GridSize1;

    float2 minTileSS = groupID.xy * tileSize;
    float2 maxTileSS = (groupID.xy + 1) * tileSize;

    float2 cornersSS[4];
    cornersSS[0] = minTileSS;
    cornersSS[1] = float2(maxTileSS.x, minTileSS.y);
    cornersSS[2] = float2(minTileSS.x, maxTileSS.y);
    cornersSS[3] = maxTileSS;

    float3 points[8];
    for (int i = 0; i < 4; ++i)
    {
        points[i * 2 + 0] = ScreenToViewDepth(cornersSS[i], 0.0f);
        points[i * 2 + 1] = ScreenToViewDepth(cornersSS[i], 1.0f);
    }

    float3 aabbMin = points[0];
    float3 aabbMax = points[0];
    for (int i = 1; i < 8; ++i) {
        aabbMin = min(aabbMin, points[i]);
        aabbMax = max(aabbMax, points[i]);
    }

    Clusters[tileIndex].MinPoint = float4(aabbMin, 0.0f);
    Clusters[tileIndex].MaxPoint = float4(aabbMax, 0.0f);
    Clusters[tileIndex].Count = 0;
}
