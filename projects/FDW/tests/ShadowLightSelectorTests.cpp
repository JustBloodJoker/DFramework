#include <pch.h>
#include <System/Shadows/ShadowLightSelector.h>

#include <algorithm>
#include <cassert>
#include <vector>

int main() {
	ShadowLightSelector selector({ 8u, 4u, 0.5f });
	std::vector<ShadowLightCandidate> candidates;
	for (std::uint32_t i = 0; i < 9u; ++i) {
		candidates.push_back({ 100u + i, i, 0u, true, 0.0f, 1.0f, 10.0f, 10.0f, 2.0f });
	}
	auto first = selector.Select(candidates, 10u);
	assert(first.SelectedGlobalLightIndices.size() == 8u);
	assert(std::find(first.SelectedGlobalLightIndices.begin(), first.SelectedGlobalLightIndices.end(), 8u) == first.SelectedGlobalLightIndices.end());

	// A marginal score perturbation cannot evict a held light during the minimum hold window.
	candidates[8].ScreenInfluence += 1.0f;
	auto held = selector.Select(candidates, 11u);
	assert(std::find(held.SelectedGlobalLightIndices.begin(), held.SelectedGlobalLightIndices.end(), 8u) == held.SelectedGlobalLightIndices.end());
	assert(held.SelectedGlobalLightIndices.size() == 8u);

	// Explicitly disabled and rectangle lights never consume RT budget and retain global indices.
	candidates[0].CastsShadows = false;
	candidates[1].LightType = 3u;
	auto filtered = selector.Select(candidates, 20u);
	assert(filtered.SelectedGlobalLightIndices.size() <= 8u);
	assert(std::find(filtered.SelectedGlobalLightIndices.begin(), filtered.SelectedGlobalLightIndices.end(), 0u) == filtered.SelectedGlobalLightIndices.end());
	assert(std::find(filtered.SelectedGlobalLightIndices.begin(), filtered.SelectedGlobalLightIndices.end(), 1u) == filtered.SelectedGlobalLightIndices.end());

	// The user-facing ShadowComponent priority participates in the stable score.
	ShadowLightSelector prioritySelector({ 1u, 0u, 0.0f });
	std::vector<ShadowLightCandidate> prioritized = {
		{ 1u, 4u, 0u, true, 0.0f, 1.0f, 10.0f, 10.0f, 2.0f },
		{ 2u, 7u, 0u, true, 2.0f, 0.1f, 1.0f, 1.0f, 20.0f }
	};
	const auto priorityResult = prioritySelector.Select(prioritized, 1u);
	assert(priorityResult.SelectedGlobalLightIndices.size() == 1u);
	assert(priorityResult.SelectedGlobalLightIndices.front() == 7u);
	return 0;
}
