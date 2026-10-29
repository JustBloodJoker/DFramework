#include <System/Shadows/ShadowRouter.h>

#include <cassert>

int main() {
	ShadowRouter router;
	auto hard = router.Resolve(1u, ShadowMode::HardShadowAtlas, { false, true, true, true, true }, 3u);
	assert(hard.Routing.UsesHardShadowAtlas);
	assert(!hard.Producers.NeedsNeuralWorkItems);

	auto rtMissing = router.Resolve(2u, ShadowMode::RayTraced, { false, true, true, false, true }, 3u);
	assert(!rtMissing.Routing.UsesRayTraced);
	assert(rtMissing.Routing.FallbackReason == ShadowFallbackReason::RayTracedUnavailable);

	auto compact = router.Resolve(3u, ShadowMode::CompactBaseline, { false, true, true, true, true }, 3u);
	assert(compact.Routing.UsesCompactBaseline);
	assert(compact.Producers.NeedsHardShadowInput);
	assert(compact.Producers.NeedsNeuralWorkItems);
	assert(compact.Producers.NeedsNeuralOutput);
	assert(!compact.Producers.NeedsNeuralInference);

	auto neural = router.Resolve(4u, ShadowMode::Neural, { false, true, true, true, true, true }, 3u);
	assert(neural.Routing.UsesNeural);
	assert(neural.Routing.UsesHardShadowAtlas);
	assert(neural.Producers.NeedsNeuralInference);

	auto unavailableNeural = router.Resolve(
		5u, ShadowMode::Neural, { false, true, true, true, true, false }, 3u);
	assert(!unavailableNeural.Routing.UsesNeural);
	assert(unavailableNeural.Routing.UsesHardShadowAtlas);
	assert(unavailableNeural.Routing.FallbackReason == ShadowFallbackReason::ModeUnavailable);
	assert(!unavailableNeural.Producers.NeedsNeuralWorkItems);

	return 0;
}
