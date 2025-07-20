#pragma once

#include <pch.h>

struct LightStruct {
    dx::XMFLOAT3 Position = { 0.0f, 5.0f, 0.0f };
    float Intensity = 50.0f;
    dx::XMFLOAT3 Color = { 1.0f, 0.95f, 0.9f };
    float Radius = 10.0f;
};

struct LightBuffer {
    int LightCount;
    dx::XMFLOAT3 CameraPos;
};
