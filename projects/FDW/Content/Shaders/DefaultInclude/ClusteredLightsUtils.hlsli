#ifndef _CLUSTERED_LIGHTS_UTILITS_HLSLI
#define _CLUSTERED_LIGHTS_UTILITS_HLSLI

uint ComputeClusterIndex(float2 fragCoord, float viewZAbs, ClusterParamsPS data)
{
    uint2 screenExtent = max(uint2(data.ScreenWidth, data.ScreenHeight), uint2(1, 1));
    uint2 gridExtent = max(uint2(data.GridSize0, data.GridSize1), uint2(1, 1));
    uint2 pixel = min((uint2)max(fragCoord, 0.0f), screenExtent - 1u);
    uint2 tileXY = min((pixel * gridExtent) / screenExtent, gridExtent - 1u);
    return tileXY.x + tileXY.y * gridExtent.x;
}

#endif
