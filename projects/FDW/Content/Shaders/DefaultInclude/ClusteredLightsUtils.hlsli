#ifndef _CLUSTERED_LIGHTS_UTILITS_HLSLI
#define _CLUSTERED_LIGHTS_UTILITS_HLSLI

uint ComputeZTile(float viewZAbs, ClusterParamsPS data)
{
    float zLog = log(viewZAbs / data.ZNear) / log(data.ZFar / data.ZNear);
    uint zTile = (uint)floor(zLog * data.GridSize2);
    return clamp(zTile, 0, data.GridSize2 - 1);
}

uint ComputeClusterIndex(float2 fragCoord, float viewZAbs, ClusterParamsPS data)
{
    float2 tileSize = float2((float)data.ScreenWidth / (float)data.GridSize0, (float)data.ScreenHeight / (float)data.GridSize1);
    uint2 tileXY = (uint2)(fragCoord / tileSize);
    tileXY.x = clamp(tileXY.x, 0, data.GridSize0- 1);
    tileXY.y = clamp(tileXY.y, 0, data.GridSize1 - 1);

    uint zTile = ComputeZTile(viewZAbs, data);

    uint tileIndex = tileXY.x + tileXY.y * data.GridSize0 + zTile    * (data.GridSize0 * data.GridSize1);
    return tileIndex;
}

#endif