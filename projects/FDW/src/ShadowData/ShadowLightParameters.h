#pragma once

#include <Windows.h>
#include <DirectXMath.h>

namespace dx = DirectX;

// Shared CPU/HLSL shadow-light wire parameters. Mirrored by Structures.hlsli.
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

    void SetFilterSettings(const HardShadowAtlasFilterSettings& settings);
    HardShadowAtlasFilterSettings GetFilterSettings() const;
};

enum class NSRTQualityMode : UINT {
    RuntimeReference = 1,
    OfflineGroundTruth = 2,
    OfflinePreviewCustom = 3,
    COUNT
};

enum class NSRTDenoisingPolicy : UINT {
    Disabled = 0,
    COUNT
};

enum class NSRTOutputDomain : UINT {
    AtlasBackedPreview = 1,
    ScreenResolutionNSOutput = 2,
    COUNT
};

inline constexpr UINT NS_RT_REFERENCE_VERSION = 1u;
inline constexpr UINT NS_RT_SEED_ALGORITHM_VERSION = 1u;
inline constexpr UINT NS_RT_BIAS_POLICY_VERSION = 1u;
inline constexpr UINT NS_RT_GEOMETRY_MODE_VERSION = 1u;
inline constexpr UINT NS_RT_EMITTER_SAMPLING_VERSION = 2u;
inline constexpr UINT NS_RT_DELTA_SAMPLE_COUNT = 1u;
inline constexpr UINT NS_RT_OFFLINE_FINITE_SAMPLE_COUNT = 256u;
inline constexpr UINT NS_RT_MAX_FINITE_SAMPLE_COUNT = 4096u;
inline constexpr UINT NS_RT_MAX_PREVIEW_SAMPLE_COUNT = NS_RT_MAX_FINITE_SAMPLE_COUNT;
inline constexpr float NS_RT_DIRECTIONAL_TMAX = 1000000.0f;

struct NSRTQualitySettings {
    NSRTQualityMode Mode = NSRTQualityMode::RuntimeReference;
    UINT BaseSeed = 0u;
    UINT PreviewFiniteEmitterSampleCount = 64u;
    NSRTDenoisingPolicy DenoisingPolicy = NSRTDenoisingPolicy::Disabled;
    uint64_t OfflineGenerationId = 1u;
};

struct NSRTQualityMetadata {
    UINT ReferenceVersion = NS_RT_REFERENCE_VERSION;
    UINT QualityMode = static_cast<UINT>(NSRTQualityMode::RuntimeReference);
    UINT BaseSeed = 0u;
    UINT DeltaEmitterSampleCount = NS_RT_DELTA_SAMPLE_COUNT;
    UINT FiniteEmitterSampleCount = 1u;
    UINT DenoisingPolicy = static_cast<UINT>(NSRTDenoisingPolicy::Disabled);
    UINT SeedAlgorithmVersion = NS_RT_SEED_ALGORITHM_VERSION;
    UINT BiasPolicyVersion = NS_RT_BIAS_POLICY_VERSION;
    UINT GeometryModeVersion = NS_RT_GEOMETRY_MODE_VERSION;
    UINT GenerationLow = 0u;
    UINT GenerationHigh = 0u;
    UINT IsOffline = 0u;
    UINT IsCanonicalGroundTruthPreset = 0u; // Effective preset/output-domain eligibility; producers may only clear it.
    UINT AccumulationPrecisionBits = 32u;
    UINT TemporalAccumulationEnabled = 0u;
    float DirectionalTMax = NS_RT_DIRECTIONAL_TMAX;
    UINT EmitterSamplingVersion = NS_RT_EMITTER_SAMPLING_VERSION;
    UINT GeometryCoverageFlags = 0u;
    UINT UnsupportedGeometryInstanceCount = 0u;
    UINT MissingAccelerationStructureInstanceCount = 0u;
};

class ShadowLightParameters {
public:
    static UINT NormalizePCFKernelSize(UINT requestedKernelSize);
    static HardShadowAtlasFilterSettings SanitizeFilterSettings(HardShadowAtlasFilterSettings settings);
    static NSRTQualitySettings SanitizeQualitySettings(NSRTQualitySettings settings);
    static NSRTQualityMetadata BuildQualityMetadata(NSRTQualitySettings settings, uint64_t generation, NSRTOutputDomain outputDomain = NSRTOutputDomain::AtlasBackedPreview);

protected:
    static bool IsCanonicalTrainingTarget(NSRTQualityMode mode, NSRTOutputDomain outputDomain);
};

struct PerLightRTReferenceParams {
    AtlasRTShadowParams Atlas;
    NSRTQualityMetadata Quality;
};

struct LightAtlasRect {
    bool Visible = false;
    dx::XMFLOAT2 MinUV;
    dx::XMFLOAT2 MaxUV;
};
