#pragma once

#include <Config/ShadowMode.h>


struct ShadowBackendAvailability {
	bool UnlitScene = false;
	bool RayTracingSupported = true;
	bool HardAtlas = false;
	bool RayTraced = false;
	bool CompactBaseline = false;
	bool Neural = false;
};

struct ShadowFrameContext {
	std::uint64_t FrameIndex = 0;
	ShadowMode RequestedMode = ShadowMode::HardShadowAtlas;
	ShadowRoutingState Routing{};
	ShadowProducerRoutingState Producers{};
	std::uint32_t LightCount = 0;
};

struct ShadowVisibilityView {
	std::uint64_t LightMetadata = 0;
	std::uint64_t ShadowParameters = 0;
	std::uint64_t WorkItems = 0;
	std::uint64_t WorkHeader = 0;
	std::uint64_t PatchRecords = 0;
	std::uint64_t Visibility = 0;
	std::uint64_t Validity = 0;
	std::uint64_t OutputHeader = 0;
	std::uint64_t NeuralOutputWorkItems = 0;
	std::uint64_t NeuralOutputOutputHeader = 0;
	std::uint64_t NeuralOutputVisibility = 0;
	std::uint64_t NeuralOutputValidity = 0;
	std::uint32_t CompactVisibilityEnabled = 0;
};

struct ShadowBackendState {
	ShadowMode RequestedMode = ShadowMode::HardShadowAtlas;
	ShadowFallbackReason FallbackReason = ShadowFallbackReason::None;
	ShadowProducerRoutingState Producers{};
};

class ShadowRouter {
public:
	ShadowFrameContext Resolve(std::uint64_t frameIndex, ShadowMode requested, ShadowBackendAvailability availability, std::uint32_t lightCount) const;
};
