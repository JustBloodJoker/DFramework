#pragma once

#include <pch.h>

struct CullingCameraStructure {
    std::array<dx::XMFLOAT4,6> CameraPlanes;
    dx::XMMATRIX ViewProjection;
    UINT InstancesCount;
    INT MipLevels;
    INT HiZWidth;
    INT HiZHeight;
};