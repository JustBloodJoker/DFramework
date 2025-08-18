#pragma once

#include <pch.h>

struct CullingCameraStructure {
    std::array<dx::XMFLOAT4,6> CameraPlanes;
    UINT     InstancesCount;
    UINT     MipLevels;
    UINT     pad1;
    UINT     pad2;
};