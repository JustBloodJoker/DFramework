#pragma once

#include <pch.h>

// Mirrored by SHADOW_ATLAS_FILTER_MODE_* in Structures.hlsli.
enum class HardShadowAtlasFilterMode : UINT {
    Nearest = 0,
    Bilinear = 1,
    PCF = 2,
    JointBilateral = 3,
    JointBilateralPCF = 4,
    Poisson16 = 5,
    JointBilateralPoisson16 = 6,
    COUNT
};

constexpr HardShadowAtlasFilterMode DEFAULT_HARD_SHADOW_ATLAS_FILTER_MODE = HardShadowAtlasFilterMode::JointBilateral;

struct HardShadowAtlasFilterSettings {
    HardShadowAtlasFilterMode Mode = DEFAULT_HARD_SHADOW_ATLAS_FILTER_MODE;
    UINT PCFKernelSize = 5;
    float FilterRadius = 1.5f;
    float DepthRejectionSharpness = 10.0f;
    float NormalRejectionSharpness = 64.0f;
    float BlackLevel = 0.5f;
    float ShadowContrast = 1.0f;
    float NoiseScale = 1.0f;
};

inline UINT NormalizeShadowPCFKernelSize(UINT requestedKernelSize) {
    if (requestedKernelSize <= 3u) return 3u;

    if (requestedKernelSize <= 5u) return 5u;

    return 16u;
}

inline HardShadowAtlasFilterSettings SanitizeHardShadowAtlasFilterSettings(HardShadowAtlasFilterSettings settings) {
    const auto modeValue = static_cast<UINT>(settings.Mode);

    if (modeValue >= static_cast<UINT>(HardShadowAtlasFilterMode::COUNT)) {
        settings.Mode = DEFAULT_HARD_SHADOW_ATLAS_FILTER_MODE;
    }

    settings.PCFKernelSize = NormalizeShadowPCFKernelSize(settings.PCFKernelSize);
    settings.FilterRadius = std::clamp(settings.FilterRadius, 0.05f, 8.0f);
    settings.DepthRejectionSharpness = std::clamp(settings.DepthRejectionSharpness, 0.01f, 512.0f);
    settings.NormalRejectionSharpness = std::clamp(settings.NormalRejectionSharpness, 0.01f, 512.0f);
    settings.BlackLevel = std::clamp(settings.BlackLevel, 0.0f, 0.98f);
    settings.ShadowContrast = std::clamp(settings.ShadowContrast, 0.1f, 4.0f);
    settings.NoiseScale = std::clamp(settings.NoiseScale, 0.0f, 4.0f);
    return settings;
}

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
    UINT FilterMode = static_cast<UINT>(DEFAULT_HARD_SHADOW_ATLAS_FILTER_MODE);
    UINT PCFKernelSize = 5u;
    float FilterRadius = 1.5f;
    float DepthRejectionSharpness = 10.0f;
    float NormalRejectionSharpness = 64.0f;
    float BlackLevel = 0.5f;
    float ShadowContrast = 1.0f;
    float NoiseScale = 1.0f;
	dx::XMMATRIX InverseViewProjectionMatrix;

    void SetFilterSettings(const HardShadowAtlasFilterSettings& settings) {
        const auto sanitized = SanitizeHardShadowAtlasFilterSettings(settings);
        FilterMode = static_cast<UINT>(sanitized.Mode);
        PCFKernelSize = sanitized.PCFKernelSize;
        FilterRadius = sanitized.FilterRadius;
        DepthRejectionSharpness = sanitized.DepthRejectionSharpness;
        NormalRejectionSharpness = sanitized.NormalRejectionSharpness;
        BlackLevel = sanitized.BlackLevel;
        ShadowContrast = sanitized.ShadowContrast;
        NoiseScale = sanitized.NoiseScale;
    }

    HardShadowAtlasFilterSettings GetFilterSettings() const {
        HardShadowAtlasFilterSettings settings{};

        if (FilterMode < static_cast<UINT>(HardShadowAtlasFilterMode::COUNT)) {
            settings.Mode = static_cast<HardShadowAtlasFilterMode>(FilterMode);
        }

        settings.PCFKernelSize = PCFKernelSize;
        settings.FilterRadius = FilterRadius;
        settings.DepthRejectionSharpness = DepthRejectionSharpness;
        settings.NormalRejectionSharpness = NormalRejectionSharpness;
        settings.BlackLevel = BlackLevel;
        settings.ShadowContrast = ShadowContrast;
        settings.NoiseScale = NoiseScale;
        return SanitizeHardShadowAtlasFilterSettings(settings);
    }
};

struct LightAtlasRect {
    bool Visible = false;
    dx::XMFLOAT2 MinUV;
    dx::XMFLOAT2 MaxUV;
};
