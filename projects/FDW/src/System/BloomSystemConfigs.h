#pragma once

#include <pch.h>

struct BloomSystemBrightPassData {
	float Threshold = 1.0f;
	dx::XMFLOAT3 padd;
};

struct BloomSystemBlurParams {
    dx::XMFLOAT2 TexelSize;
    int Horizontal;
    int padd;
};

struct BloomSystemCompositeData {
    float BloomIntensity = 1.0f;
    dx::XMFLOAT3 padd;
};

enum BloomBlurType {
    Horizontal = 0x1,
    Vertical = 0x2,
    Both = 0x3
};