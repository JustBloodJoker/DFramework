#pragma once

#include <pch.h>

struct CullingCameraStructure {
    std::array<dx::XMFLOAT4,6> CameraPlanes;
    UINT     InstancesCount;
    UINT     pad0;
    UINT     pad1;
    UINT     pad2;
};