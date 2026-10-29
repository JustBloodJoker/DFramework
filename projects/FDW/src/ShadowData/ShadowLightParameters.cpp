#include "../pch.h"
#include "ShadowLightParameters.h"

UINT ShadowLightParameters::NormalizePCFKernelSize(UINT requestedKernelSize) {
	if (requestedKernelSize <= 3u) return 3u;
	
	if (requestedKernelSize <= 5u) return 5u;
	
	return 16u;
}

HardShadowAtlasFilterSettings ShadowLightParameters::SanitizeFilterSettings(HardShadowAtlasFilterSettings settings) {
	if (static_cast<UINT>(settings.Mode) >= static_cast<UINT>(HardShadowAtlasFilterMode::COUNT)) {
		settings.Mode = DEFAULT_HARD_SHADOW_ATLAS_FILTER_MODE;
	}
	
	settings.PCFKernelSize = NormalizePCFKernelSize(settings.PCFKernelSize);
	settings.FilterRadius = std::clamp(settings.FilterRadius, 0.05f, 8.0f);
	settings.DepthRejectionSharpness = std::clamp(settings.DepthRejectionSharpness, 0.01f, 512.0f);
	settings.NormalRejectionSharpness = std::clamp(settings.NormalRejectionSharpness, 0.01f, 512.0f);
	settings.BlackLevel = std::clamp(settings.BlackLevel, 0.0f, 0.98f);
	settings.ShadowContrast = std::clamp(settings.ShadowContrast, 0.1f, 4.0f);
	settings.NoiseScale = std::clamp(settings.NoiseScale, 0.0f, 4.0f);
	
	return settings;
}

void AtlasRTShadowParams::SetFilterSettings(const HardShadowAtlasFilterSettings& settings) {
	const auto sanitized = ShadowLightParameters::SanitizeFilterSettings(settings);
	
	FilterMode = static_cast<UINT>(sanitized.Mode);
	PCFKernelSize = sanitized.PCFKernelSize;
	FilterRadius = sanitized.FilterRadius;
	DepthRejectionSharpness = sanitized.DepthRejectionSharpness;
	NormalRejectionSharpness = sanitized.NormalRejectionSharpness;
	BlackLevel = sanitized.BlackLevel;
	ShadowContrast = sanitized.ShadowContrast;
	NoiseScale = sanitized.NoiseScale;
}

HardShadowAtlasFilterSettings AtlasRTShadowParams::GetFilterSettings() const {
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
	
	return ShadowLightParameters::SanitizeFilterSettings(settings);
}

bool ShadowLightParameters::IsCanonicalTrainingTarget(NSRTQualityMode mode, NSRTOutputDomain outputDomain) {
	return mode == NSRTQualityMode::OfflineGroundTruth && outputDomain == NSRTOutputDomain::ScreenResolutionNSOutput;
}

NSRTQualitySettings ShadowLightParameters::SanitizeQualitySettings(NSRTQualitySettings settings) {
	if (static_cast<UINT>(settings.Mode) == 0u || static_cast<UINT>(settings.Mode) >= static_cast<UINT>(NSRTQualityMode::COUNT)) {
		settings.Mode = NSRTQualityMode::RuntimeReference;
	}
	
	settings.PreviewFiniteEmitterSampleCount = std::clamp(settings.PreviewFiniteEmitterSampleCount, 1u, NS_RT_MAX_PREVIEW_SAMPLE_COUNT);
	settings.DenoisingPolicy = NSRTDenoisingPolicy::Disabled;
	settings.OfflineGenerationId = std::max<std::uint64_t>(settings.OfflineGenerationId, 1u);
	
	return settings;
}

NSRTQualityMetadata ShadowLightParameters::BuildQualityMetadata( NSRTQualitySettings settings, std::uint64_t generation, NSRTOutputDomain outputDomain) {
	settings = SanitizeQualitySettings(settings);
	
	NSRTQualityMetadata metadata{};
	metadata.QualityMode = static_cast<UINT>(settings.Mode);
	metadata.BaseSeed = settings.BaseSeed;
	metadata.DenoisingPolicy = static_cast<UINT>(settings.DenoisingPolicy);
	metadata.GenerationLow = static_cast<UINT>(generation & 0xFFFFFFFFull);
	metadata.GenerationHigh = static_cast<UINT>(generation >> 32u);
	metadata.IsOffline = settings.Mode == NSRTQualityMode::RuntimeReference ? 0u : 1u;
	metadata.IsCanonicalGroundTruthPreset = IsCanonicalTrainingTarget(settings.Mode, outputDomain) ? 1u : 0u;
	
	if (settings.Mode == NSRTQualityMode::OfflineGroundTruth) {
		metadata.FiniteEmitterSampleCount = std::max(NS_RT_OFFLINE_FINITE_SAMPLE_COUNT, settings.PreviewFiniteEmitterSampleCount);
	}
	else if (settings.Mode == NSRTQualityMode::OfflinePreviewCustom) {
		metadata.FiniteEmitterSampleCount = settings.PreviewFiniteEmitterSampleCount;
	}
	return metadata;
}

