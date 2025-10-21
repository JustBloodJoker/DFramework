#pragma once

#include <pch.h>

struct LightAtlasMeta {
    UINT LightIndex;
    UINT AtlasOffsetX;
    UINT AtlasOffsetY;
    UINT AtlasWidth;
    UINT AtlasHeight;
    UINT padd1;
    UINT padd2;
    UINT padd3;
    float ScreenMinU;
    float ScreenMinV;
    float ScreenMaxU;
    float ScreenMaxV;
};

struct AtlasRTShadowParams {
    UINT ScreenWidth;
    UINT ScreenHeight;
    UINT AtlasWidth;
    UINT AtlasHeight;
};

struct LightAtlasRect {
    bool Visible = false;
    dx::XMFLOAT2 MinUV;
    dx::XMFLOAT2 MaxUV;
};