#pragma once

#include <Windows.h>

enum class ShadowMode : UINT {
	HardShadowAtlas = 0,
	RayTraced = 1,
	Neural = 2,
	CompactBaseline = 3,
	COUNT
};

inline constexpr ShadowMode DEFAULT_SHADOW_MODE = ShadowMode::HardShadowAtlas;

enum class ShadowFallbackReason : UINT {
	None = 0,
	UnlitScene = 1,
	ModeUnavailable = 2,
	RayTracingUnavailable = 3,
	HardAtlasUnavailable = 4,
	RayTracedUnavailable = 5
};

struct ShadowRoutingState {
	ShadowMode RequestedMode = ShadowMode::HardShadowAtlas;
	ShadowFallbackReason FallbackReason = ShadowFallbackReason::None;
	bool UsesHardShadowAtlas = false;
	bool UsesRayTraced = false;
	bool UsesCompactBaseline = false;
	bool UsesNeural = false;
};

enum class NeuralShadowActivationReason : UINT {
	None = 0,
	CompactBaseline = 1,
	Neural = 2
};

struct ShadowProducerRoutingState {
	bool NeedsHardShadowInput = false;
	bool NeedsRayTracedVisibility = false;
	bool NeedsShadowMetadata = false;
	bool NeedsAccelerationStructure = false;
	bool NeedsNeuralWorkItems = false;
	bool NeedsNeuralFeaturePreparation = false;
	bool NeedsNeuralOutput = false;
	bool NeedsNeuralInference = false;
	NeuralShadowActivationReason NeuralActivationReason = NeuralShadowActivationReason::None;
};

class ShadowRouting {
public:
	static bool IsKnownMode(ShadowMode mode);
	static ShadowRoutingState Resolve(
		ShadowMode requestedMode,
		bool isUnlitScene,
		bool isRayTracingSupported,
		bool isHardAtlasReady,
		bool isRayTracedReady,
		bool isCompactBaselineReady,
		bool isNeuralReady = false);
	static ShadowProducerRoutingState ResolveProducers(const ShadowRoutingState& routing);
	static bool UsesPerLightVisibility(const ShadowRoutingState& routing);
	static const char* GetActivationReasonLabel(NeuralShadowActivationReason reason);
	static const char* GetFallbackReasonLabel(ShadowFallbackReason reason);

protected:
	static bool IsImplementedMode(ShadowMode mode);
	static bool NeedsShadowMetadata(const ShadowRoutingState& routing);
	static bool NeedsAccelerationStructure(const ShadowRoutingState& routing);
	static bool NeedsNeuralWorkItems(const ShadowRoutingState& routing);
	static bool NeedsNeuralFeaturePreparation(const ShadowRoutingState& routing);
	static bool NeedsNeuralOutput(const ShadowRoutingState& routing);
	static bool NeedsNeuralInference(const ShadowRoutingState& routing);
};
