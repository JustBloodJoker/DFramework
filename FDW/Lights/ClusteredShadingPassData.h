#pragma once

#include <pch.h>
#include <MainRenderer/GlobalConfig.h>

struct Cluster
{
    dx::XMFLOAT4 MinPoint;
    dx::XMFLOAT4  MaxPoint;
    UINT Count;
    UINT LightIndices[CLUSTERED_MAX_LIGHTS_PER_CLUSTER];
    UINT padd[3];
};

struct ClusterViewParams
{
    dx::XMFLOAT4X4 ViewMatrix;
    int LightCount;
};

struct ClusterParams
{
    float ZNear;
    float ZFar;
    uint32_t GridSize0;
    uint32_t GridSize1;

    uint32_t GridSize2;
    uint32_t ScreenWidth;
    uint32_t ScreenHeight;
    uint32_t _pad0;

    dx::XMFLOAT4X4 InverseProjection;
};

struct ClusterParamsPS {
    float ZNear;
    float ZFar;
    uint32_t GridSize0;
    uint32_t GridSize1;

    uint32_t GridSize2;
    uint32_t ScreenWidth;
    uint32_t ScreenHeight;
    uint32_t _pad0;

    dx::XMFLOAT4X4 ViewMatrix;
};