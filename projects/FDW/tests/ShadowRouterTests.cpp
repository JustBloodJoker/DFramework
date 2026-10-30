#include <System/Shadows/ShadowRouter.h>

int main() {
	ShadowRouter router;
	auto hard = router.Resolve(1u, ShadowMode::HardShadowAtlas, { false, true, true, true, true }, 3u);
	if (!hard.Routing.UsesHardShadowAtlas) return __LINE__;
	if (hard.Producers.NeedsNeuralWorkItems) return __LINE__;

	auto rtMissing = router.Resolve(2u, ShadowMode::RayTraced, { false, true, true, false, true }, 3u);
	if (rtMissing.Routing.UsesRayTraced) return __LINE__;
	if (rtMissing.Routing.FallbackReason != ShadowFallbackReason::RayTracedUnavailable) return __LINE__;

	auto compact = router.Resolve(3u, ShadowMode::CompactBaseline, { false, true, true, true, true }, 3u);
	if (!compact.Routing.UsesCompactBaseline) return __LINE__;
	if (!compact.Producers.NeedsHardShadowInput) return __LINE__;
	if (!compact.Producers.NeedsNeuralWorkItems) return __LINE__;
	if (!compact.Producers.NeedsNeuralOutput) return __LINE__;
	if (compact.Producers.NeedsNeuralInference) return __LINE__;

	auto neural = router.Resolve(4u, ShadowMode::Neural, { false, true, true, true, true, true }, 3u);
	if (!neural.Routing.UsesNeural) return __LINE__;
	if (!neural.Routing.UsesHardShadowAtlas) return __LINE__;
	if (!neural.Producers.NeedsNeuralInference) return __LINE__;

	auto unavailableNeural = router.Resolve(
		5u, ShadowMode::Neural, { false, true, true, true, true, false }, 3u);
	if (unavailableNeural.Routing.UsesNeural) return __LINE__;
	if (!unavailableNeural.Routing.UsesHardShadowAtlas) return __LINE__;
	if (unavailableNeural.Routing.FallbackReason != ShadowFallbackReason::ModeUnavailable) return __LINE__;
	if (unavailableNeural.Producers.NeedsNeuralWorkItems) return __LINE__;

	return 0;
}
