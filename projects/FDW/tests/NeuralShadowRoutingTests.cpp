#include <Config/ShadowMode.h>

class RoutingTest {
public:
	static int Run();

protected:
	static bool HasNoNeuralStages(const ShadowProducerRoutingState& routing) {
		return !routing.NeedsNeuralWorkItems &&
			!routing.NeedsNeuralFeaturePreparation &&
			!routing.NeedsNeuralOutput &&
			!routing.NeedsNeuralInference &&
			routing.NeuralActivationReason == NeuralShadowActivationReason::None;
	}
};

int RoutingTest::Run() {
	const auto hard = ShadowRouting::Resolve(
		ShadowMode::HardShadowAtlas, false, true, true, true, true);
	const auto hardProducers = ShadowRouting::ResolveProducers(hard);
	if (!hard.UsesHardShadowAtlas || hard.UsesRayTraced || hard.UsesCompactBaseline) return 1;
	if (!hardProducers.NeedsHardShadowInput || !hardProducers.NeedsShadowMetadata ||
		!hardProducers.NeedsAccelerationStructure || !HasNoNeuralStages(hardProducers)) return 2;

	const auto rayTraced = ShadowRouting::Resolve(
		ShadowMode::RayTraced, false, true, true, true, true);
	const auto rayTracedProducers = ShadowRouting::ResolveProducers(rayTraced);
	if (!rayTraced.UsesRayTraced || rayTraced.UsesHardShadowAtlas || rayTraced.UsesCompactBaseline) return 3;
	if (rayTracedProducers.NeedsHardShadowInput || !rayTracedProducers.NeedsRayTracedVisibility ||
		!rayTracedProducers.NeedsShadowMetadata || !rayTracedProducers.NeedsAccelerationStructure ||
		!HasNoNeuralStages(rayTracedProducers)) return 4;

	const auto compact = ShadowRouting::Resolve(
		ShadowMode::CompactBaseline, false, true, true, true, true);
	const auto compactProducers = ShadowRouting::ResolveProducers(compact);
	if (!compact.UsesHardShadowAtlas || !compact.UsesCompactBaseline || compact.UsesRayTraced) return 5;
	if (!compactProducers.NeedsHardShadowInput || compactProducers.NeedsRayTracedVisibility ||
		!compactProducers.NeedsShadowMetadata || !compactProducers.NeedsAccelerationStructure ||
		!compactProducers.NeedsNeuralWorkItems || !compactProducers.NeedsNeuralFeaturePreparation ||
		!compactProducers.NeedsNeuralOutput || compactProducers.NeedsNeuralInference ||
		compactProducers.NeuralActivationReason != NeuralShadowActivationReason::CompactBaseline) return 6;

	const auto neural = ShadowRouting::Resolve(
		ShadowMode::Neural, false, true, true, true, true, true);
	const auto neuralProducers = ShadowRouting::ResolveProducers(neural);
	if (!neural.UsesNeural || !neural.UsesHardShadowAtlas || neural.UsesRayTraced ||
		neural.UsesCompactBaseline || !neuralProducers.NeedsHardShadowInput ||
		!neuralProducers.NeedsNeuralWorkItems || !neuralProducers.NeedsNeuralFeaturePreparation ||
		!neuralProducers.NeedsNeuralOutput || !neuralProducers.NeedsNeuralInference ||
		neuralProducers.NeuralActivationReason != NeuralShadowActivationReason::Neural) return 7;

	const auto unavailableNeural = ShadowRouting::Resolve(
		ShadowMode::Neural, false, true, true, true, true, false);
	if (unavailableNeural.FallbackReason != ShadowFallbackReason::ModeUnavailable ||
		!unavailableNeural.UsesHardShadowAtlas || unavailableNeural.UsesNeural ||
		!HasNoNeuralStages(ShadowRouting::ResolveProducers(unavailableNeural))) return 14;

	const auto neuralHardMissing = ShadowRouting::Resolve(
		ShadowMode::Neural, false, true, false, true, true, false);
	if (neuralHardMissing.FallbackReason != ShadowFallbackReason::ModeUnavailable ||
		!neuralHardMissing.UsesRayTraced || neuralHardMissing.UsesNeural ||
		!HasNoNeuralStages(ShadowRouting::ResolveProducers(neuralHardMissing))) return 15;

	const auto unlit = ShadowRouting::Resolve(
		ShadowMode::CompactBaseline, true, true, true, true, true);
	const auto unlitProducers = ShadowRouting::ResolveProducers(unlit);
	if (unlit.FallbackReason != ShadowFallbackReason::UnlitScene ||
		unlitProducers.NeedsShadowMetadata || unlitProducers.NeedsAccelerationStructure ||
		unlitProducers.NeedsHardShadowInput || !HasNoNeuralStages(unlitProducers)) return 8;

	const auto noDxr = ShadowRouting::Resolve(
		ShadowMode::HardShadowAtlas, false, false, false, false, false);
	if (noDxr.FallbackReason != ShadowFallbackReason::RayTracingUnavailable ||
		!HasNoNeuralStages(ShadowRouting::ResolveProducers(noDxr))) return 9;

	const auto noHardAtlas = ShadowRouting::Resolve(
		ShadowMode::HardShadowAtlas, false, true, false, true, true);
	if (noHardAtlas.FallbackReason != ShadowFallbackReason::HardAtlasUnavailable ||
		!HasNoNeuralStages(ShadowRouting::ResolveProducers(noHardAtlas))) return 10;

	const auto noRayTraced = ShadowRouting::Resolve(
		ShadowMode::RayTraced, false, true, true, false, true);
	if (noRayTraced.FallbackReason != ShadowFallbackReason::RayTracedUnavailable ||
		!HasNoNeuralStages(ShadowRouting::ResolveProducers(noRayTraced))) return 11;

	const auto noCompact = ShadowRouting::Resolve(
		ShadowMode::CompactBaseline, false, true, true, true, false);
	if (noCompact.FallbackReason != ShadowFallbackReason::ModeUnavailable ||
		!HasNoNeuralStages(ShadowRouting::ResolveProducers(noCompact))) return 12;

	return 0;
}

int main() { return RoutingTest::Run(); }
