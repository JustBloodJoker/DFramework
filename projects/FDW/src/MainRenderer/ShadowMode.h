#pragma once

#include <pch.h>

//TODO : move to other place

enum class ShadowMode : UINT {
	HardShadowAtlas = 0,
	RayTraced = 1,
	Neural = 2,
	COUNT
};

inline constexpr ShadowMode DEFAULT_SHADOW_MODE = ShadowMode::HardShadowAtlas;

enum class ShadowFallbackReason : UINT {
	None = 0,
	UnlitScene = 1,
	ModeUnavailable = 2,
	RayTracingUnavailable = 3,
	HardAtlasUnavailable = 4
};

struct ShadowRoutingState {
	ShadowMode RequestedMode = ShadowMode::HardShadowAtlas;
	ShadowFallbackReason FallbackReason = ShadowFallbackReason::None;
	bool UsesHardShadowAtlas = false;
};


constexpr bool IsKnownShadowMode(ShadowMode mode) { return static_cast<UINT>(mode) < static_cast<UINT>(ShadowMode::COUNT); }

constexpr bool IsImplementedShadowMode(ShadowMode mode) { return mode == ShadowMode::HardShadowAtlas; }

constexpr ShadowRoutingState ResolveShadowRouting( ShadowMode requestedMode, bool isUnlitScene, bool isRayTracingSupported, bool isHardAtlasReady) {
	
	if (isUnlitScene) {
		return { requestedMode, ShadowFallbackReason::UnlitScene, false };
	}
	
	if (requestedMode != ShadowMode::HardShadowAtlas) {
		return { requestedMode, ShadowFallbackReason::ModeUnavailable, false };
	}
	
	if (!isRayTracingSupported) {
		return { requestedMode, ShadowFallbackReason::RayTracingUnavailable, false };
	}
	
	if (!isHardAtlasReady) {
		return { requestedMode, ShadowFallbackReason::HardAtlasUnavailable, false };
	}
	
	return { requestedMode, ShadowFallbackReason::None, true };
}

constexpr const char* GetShadowFallbackReasonLabel(ShadowFallbackReason reason) {
	switch (reason) {
	
	case ShadowFallbackReason::None:
		return "none";
	case ShadowFallbackReason::UnlitScene:
		return "unlit scene";
	case ShadowFallbackReason::ModeUnavailable:
		return "selected backend unavailable";
	case ShadowFallbackReason::RayTracingUnavailable:
		return "DXR unavailable";
	case ShadowFallbackReason::HardAtlasUnavailable:
		return "hard-atlas backend unavailable";
	default:
		return "unknown";
	
	}
}

static_assert(IsImplementedShadowMode(DEFAULT_SHADOW_MODE));
static_assert(!IsImplementedShadowMode(ShadowMode::RayTraced));
static_assert(!IsImplementedShadowMode(ShadowMode::Neural));

static_assert(!IsKnownShadowMode(ShadowMode::COUNT));

static_assert(ResolveShadowRouting(DEFAULT_SHADOW_MODE, false, true, true).UsesHardShadowAtlas);
static_assert(ResolveShadowRouting(DEFAULT_SHADOW_MODE, false, false, false).FallbackReason == ShadowFallbackReason::RayTracingUnavailable);
static_assert(ResolveShadowRouting(ShadowMode::Neural, false, true, true).FallbackReason == ShadowFallbackReason::ModeUnavailable);
static_assert(ResolveShadowRouting(DEFAULT_SHADOW_MODE, true, true, true).FallbackReason == ShadowFallbackReason::UnlitScene);
