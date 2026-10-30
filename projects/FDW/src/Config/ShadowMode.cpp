#include <pch.h>

#include <Config/ShadowMode.h>

bool ShadowRouting::IsKnownMode(ShadowMode mode) {
	return static_cast<UINT>(mode) < static_cast<UINT>(ShadowMode::COUNT);
}

bool ShadowRouting::IsImplementedMode(ShadowMode mode) {
	return mode == ShadowMode::HardShadowAtlas || mode == ShadowMode::RayTraced ||
		mode == ShadowMode::Neural || mode == ShadowMode::CompactBaseline;
}

ShadowRoutingState ShadowRouting::Resolve(ShadowMode requestedMode,bool isUnlitScene, bool isRayTracingSupported, bool isHardAtlasReady, bool isRayTracedReady, bool isCompactBaselineReady, bool isNeuralReady) {
	if (isUnlitScene) return { requestedMode, ShadowFallbackReason::UnlitScene, false, false };
	
	if (!IsImplementedMode(requestedMode)) {
		return { requestedMode, ShadowFallbackReason::ModeUnavailable, false, false };
	}
	
	if (!isRayTracingSupported) {
		return { requestedMode, ShadowFallbackReason::RayTracingUnavailable, false, false };
	}
	
	if (requestedMode == ShadowMode::HardShadowAtlas) {
		return isHardAtlasReady
			? ShadowRoutingState{ requestedMode, ShadowFallbackReason::None, true, false, false }
			: ShadowRoutingState{ requestedMode, ShadowFallbackReason::HardAtlasUnavailable, false, false, false };
	}
	
	if (requestedMode == ShadowMode::CompactBaseline) {
		return isHardAtlasReady && isCompactBaselineReady
			? ShadowRoutingState{ requestedMode, ShadowFallbackReason::None, true, false, true }
			: ShadowRoutingState{ requestedMode, ShadowFallbackReason::ModeUnavailable, false, false, false };
	}
	
	if (requestedMode == ShadowMode::Neural) {
		if (isHardAtlasReady && isCompactBaselineReady && isNeuralReady) {
			return { requestedMode, ShadowFallbackReason::None, true, false, false, true };
		}
		if (isHardAtlasReady) {
			return { requestedMode, ShadowFallbackReason::ModeUnavailable, true, false, false, false };
		}
		if (isRayTracedReady) {
			return { requestedMode, ShadowFallbackReason::ModeUnavailable, false, true, false, false };
		}
		return { requestedMode, ShadowFallbackReason::ModeUnavailable, false, false, false, false };
	}
	
	return isRayTracedReady
		? ShadowRoutingState{ requestedMode, ShadowFallbackReason::None, false, true, false }
		: ShadowRoutingState{ requestedMode, ShadowFallbackReason::RayTracedUnavailable, false, false, false };
}

bool ShadowRouting::UsesPerLightVisibility(const ShadowRoutingState& routing) {
	return routing.UsesHardShadowAtlas || routing.UsesRayTraced;
}

ShadowProducerRoutingState ShadowRouting::ResolveProducers(const ShadowRoutingState& routing) {
	const bool needsPerLightVisibility = UsesPerLightVisibility(routing);
	const bool needsCompactOutput = routing.UsesCompactBaseline || routing.UsesNeural;
	const auto activationReason = routing.UsesNeural
			? NeuralShadowActivationReason::Neural
			: routing.UsesCompactBaseline
				? NeuralShadowActivationReason::CompactBaseline
				: NeuralShadowActivationReason::None;
	return {
		routing.UsesHardShadowAtlas,
		routing.UsesRayTraced,
		needsPerLightVisibility,
		needsPerLightVisibility,
		needsCompactOutput,
		needsCompactOutput,
		needsCompactOutput,
		routing.UsesNeural,
		activationReason
	};
}

const char* ShadowRouting::GetActivationReasonLabel(NeuralShadowActivationReason reason) {
	switch (reason) {
	case NeuralShadowActivationReason::None: return "inactive";
	case NeuralShadowActivationReason::CompactBaseline: return "compact baseline";
	case NeuralShadowActivationReason::Neural: return "neural inference";
	default: return "unknown";
	}
}

const char* ShadowRouting::GetFallbackReasonLabel(ShadowFallbackReason reason) {
	switch (reason) {
	case ShadowFallbackReason::None: return "none";
	case ShadowFallbackReason::UnlitScene: return "unlit scene";
	case ShadowFallbackReason::ModeUnavailable: return "selected backend unavailable";
	case ShadowFallbackReason::RayTracingUnavailable: return "DXR unavailable";
	case ShadowFallbackReason::HardAtlasUnavailable: return "hard-atlas backend unavailable";
	case ShadowFallbackReason::RayTracedUnavailable: return "per-light RT backend unavailable";
	default: return "unknown";
	}
}
