#include "Structures.hlsli"

RWStructuredBuffer<Cluster> Clusters : register(u0);
ConstantBuffer<ClusterParams> ClusterData: register(b0);

float3 LineIntersectionWithZPlane(float3 startPoint, float3 endPoint, float zDistance)
{
    float3 direction = endPoint - startPoint;
    float3 normal = float3(0, 0, -1);
    float t = (zDistance - dot(normal, startPoint)) / dot(normal, direction);
    return startPoint + t * direction;
}

float3 ScreenToView(float2 screenCoord)
{
    float2 scrDim = float2(ClusterData.ScreenWidth,ClusterData.ScreenHeight);
    float4 ndc = float4((screenCoord / scrDim) * 2.0f - 1.0f, -1.0f, 1.0f);
    float4 viewCoord = mul(ndc, ClusterData.InverseProjection);
    viewCoord /= viewCoord.w;
    return viewCoord.xyz;
}


[numthreads(1, 1, 1)]
void CS(uint3 groupID : SV_DispatchThreadID)
{
    uint tileIndex = groupID.x +
        (groupID.y * ClusterData.GridSize0) +
        (groupID.z * ClusterData.GridSize0 * ClusterData.GridSize1);

    float2 tileSize =float2(0,0);
    tileSize.x = ClusterData.ScreenWidth / (float)ClusterData.GridSize0; 
    tileSize.y = ClusterData.ScreenHeight / (float)ClusterData.GridSize1; 

    float2 minTileSS = groupID.xy * tileSize;
    float2 maxTileSS = (groupID.xy + 1) * tileSize;

    float3 minTileVS = ScreenToView(minTileSS);
    float3 maxTileVS = ScreenToView(maxTileSS);

    float planeNear = ClusterData.ZNear * pow(ClusterData.ZFar / ClusterData.ZNear, groupID.z / (float)ClusterData.GridSize2);
    float planeFar = ClusterData.ZNear * pow(ClusterData.ZFar /ClusterData.ZNear, (groupID.z + 1) / (float)ClusterData.GridSize2);

    float3 minNear = LineIntersectionWithZPlane(float3(0, 0, 0), minTileVS, planeNear);
    float3 minFar  = LineIntersectionWithZPlane(float3(0, 0, 0), minTileVS, planeFar);
    float3 maxNear = LineIntersectionWithZPlane(float3(0, 0, 0), maxTileVS, planeNear);
    float3 maxFar  = LineIntersectionWithZPlane(float3(0, 0, 0), maxTileVS, planeFar);

    Clusters[tileIndex].MinPoint = float4(min(minNear, minFar), 0.0f);
    Clusters[tileIndex].MaxPoint = float4(max(maxNear, maxFar), 0.0f);
    Clusters[tileIndex].Count = 0;
}
