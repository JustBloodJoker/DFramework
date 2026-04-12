#pragma once

#include <pch.h>

enum class ShadowUpscaleMode : UINT {
    Point = 0,
    Bilinear = 1,
    PCF = 2,
    JointBilateral = 3,
    JointBilateralPCF = 4,
    Poisson16 = 5,
    JointBilateralPoisson16 = 6,
    COUNT
};

struct ShadowUpscaleSettings {
    ShadowUpscaleMode Mode = ShadowUpscaleMode::JointBilateral;
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

inline ShadowUpscaleSettings SanitizeShadowUpscaleSettings(ShadowUpscaleSettings settings) {
    const auto modeValue = static_cast<UINT>(settings.Mode);
    if (modeValue >= static_cast<UINT>(ShadowUpscaleMode::COUNT)) {
        settings.Mode = ShadowUpscaleMode::JointBilateral;
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
    UINT UpscaleMode = static_cast<UINT>(ShadowUpscaleMode::JointBilateral);
    UINT PCFKernelSize = 5u;
    float FilterRadius = 1.5f;
    float DepthRejectionSharpness = 10.0f;
    float NormalRejectionSharpness = 64.0f;
    float BlackLevel = 0.5f;
    float ShadowContrast = 1.0f;
    float NoiseScale = 1.0f;
	dx::XMMATRIX InverseViewProjectionMatrix;

    void SetUpscaleSettings(const ShadowUpscaleSettings& settings) {
        const auto sanitized = SanitizeShadowUpscaleSettings(settings);
        UpscaleMode = static_cast<UINT>(sanitized.Mode);
        PCFKernelSize = sanitized.PCFKernelSize;
        FilterRadius = sanitized.FilterRadius;
        DepthRejectionSharpness = sanitized.DepthRejectionSharpness;
        NormalRejectionSharpness = sanitized.NormalRejectionSharpness;
        BlackLevel = sanitized.BlackLevel;
        ShadowContrast = sanitized.ShadowContrast;
        NoiseScale = sanitized.NoiseScale;
    }

    ShadowUpscaleSettings GetUpscaleSettings() const {
        ShadowUpscaleSettings settings{};
        if (UpscaleMode < static_cast<UINT>(ShadowUpscaleMode::COUNT)) {
            settings.Mode = static_cast<ShadowUpscaleMode>(UpscaleMode);
        }
        settings.PCFKernelSize = PCFKernelSize;
        settings.FilterRadius = FilterRadius;
        settings.DepthRejectionSharpness = DepthRejectionSharpness;
        settings.NormalRejectionSharpness = NormalRejectionSharpness;
        settings.BlackLevel = BlackLevel;
        settings.ShadowContrast = ShadowContrast;
        settings.NoiseScale = NoiseScale;
        return SanitizeShadowUpscaleSettings(settings);
    }
};

struct LightAtlasRect {
    bool Visible = false;
    dx::XMFLOAT2 MinUV;
    dx::XMFLOAT2 MaxUV;
};
