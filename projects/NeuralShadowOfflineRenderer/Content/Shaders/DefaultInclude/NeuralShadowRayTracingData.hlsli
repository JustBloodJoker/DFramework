#ifndef NEURAL_SHADOW_RAY_TRACING_DATA_HLSLI
#define NEURAL_SHADOW_RAY_TRACING_DATA_HLSLI

#include "Structures.hlsli"

static const uint NS_RT_REFERENCE_VERSION = 1u;
static const uint NS_RT_QUALITY_MODE_RUNTIME_REFERENCE = 1u;
static const uint NS_RT_QUALITY_MODE_OFFLINE_GROUND_TRUTH = 2u;
static const uint NS_RT_QUALITY_MODE_OFFLINE_PREVIEW_CUSTOM = 3u;
static const uint NS_RT_OFFLINE_MIN_FINITE_SPP = 256u;
static const uint NS_RT_OFFLINE_MAX_FINITE_SPP = 4096u;
static const uint NS_RT_SEED_ALGORITHM_VERSION = 1u;
static const uint NS_RT_BIAS_POLICY_VERSION = 1u;
static const uint NS_RT_GEOMETRY_MODE_VERSION = 1u;
static const uint NS_RT_EMITTER_SAMPLING_VERSION = 2u;

struct NSRTQualityMetadata
{
    uint ReferenceVersion;
    uint QualityMode;
    uint BaseSeed;
    uint DeltaEmitterSampleCount;
    uint FiniteEmitterSampleCount;
    uint DenoisingPolicy;
    uint SeedAlgorithmVersion;
    uint BiasPolicyVersion;
    uint GeometryModeVersion;
    uint GenerationLow;
    uint GenerationHigh;
    uint IsOffline;
    uint IsCanonicalGroundTruthPreset;
    uint AccumulationPrecisionBits;
    uint TemporalAccumulationEnabled;
    float DirectionalTMax;
    uint EmitterSamplingVersion;
    uint GeometryCoverageFlags;
    uint UnsupportedGeometryInstanceCount;
    uint MissingAccelerationStructureInstanceCount;
};

struct PerLightRTReferenceParams
{
    ShadowAtlasParams Atlas;
    NSRTQualityMetadata Quality;
};

#endif
