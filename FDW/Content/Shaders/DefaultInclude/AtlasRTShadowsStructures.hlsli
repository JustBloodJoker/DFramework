#ifndef _ATLAS_RT_SHADOWS_STRUCTURES_HLSLI_
#define _ATLAS_RT_SHADOWS_STRUCTURES_HLSLI_

struct AtlasRTShadowParams {
    uint AtlasWidth;
    uint AtlasHeight;
    uint AtlasTileSize;
    uint MaxLightCount;
    float CameraNear;
    float CameraFar;
    float ScreenWidth;
    float ScreenHeight;
};

struct LightAtlasMeta {
    uint LightIndex;
    uint AtlasOffsetX;
    uint AtlasOffsetY;
    uint AtlasWidth;
    float2 ScreenMinUV;
    float2 ScreenMaxUV;
    uint AtlasHeight;
    uint Packed0;
    uint Packed1;
    uint Padd;
};

#endif