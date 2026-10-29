#include "pch.h"

#include <System/Shadows/ShadowLightSelector.h>

bool ShadowLightSelector::IsRayTracedLightTypeSupported(std::uint32_t lightType) noexcept {
	// Point, spot and directional are supported. Rectangle remains experimental.
	return lightType <= 2u;
}

ShadowLightSelector::ShadowLightSelector(ShadowLightSelectorConfig config) : m_xConfig(config) { }

const ShadowLightSelectorConfig& ShadowLightSelector::GetConfig() const noexcept {
	return m_xConfig;
}

float ShadowLightSelector::Score(const ShadowLightCandidate& candidate, std::uint64_t frameIndex) const {
	const float safeDistance = std::max(0.0f, candidate.Distance);
	const float distanceWeight = 1.0f / (1.0f + safeDistance);
	const float contribution = std::max(0.0f, candidate.Intensity) * std::max(0.0f, candidate.Range) * distanceWeight;

	float result = candidate.ShadowPriority * 1000.0f + std::max(0.0f, candidate.ScreenInfluence) * 100.0f + std::log2(1.0f + contribution);

	const auto it = m_mHistory.find(candidate.StableId);
	if (it != m_mHistory.end() && it->second.WasSelected) {
		result += m_xConfig.HysteresisBonus;

		if (frameIndex - it->second.SelectedAtFrame < m_xConfig.MinimumHoldFrames) {
			result += 100000.0f;
		}
	}
	return result;
}

ShadowLightSelectionResult ShadowLightSelector::Select(const std::vector<ShadowLightCandidate>& candidates,std::uint64_t frameIndex) {
	ShadowLightSelectionResult result;
	result.Entries.reserve(candidates.size());

	for (const auto& candidate : candidates) {
		ShadowLightSelectionEntry entry{};
		entry.StableId = candidate.StableId;
		entry.GlobalLightIndex = candidate.GlobalLightIndex;

		if (!candidate.CastsShadows) {
			entry.Reason = ShadowLightSelectionReason::ShadowsDisabled;
		}
		else if (!IsRayTracedLightTypeSupported(candidate.LightType)) {
			entry.Reason = ShadowLightSelectionReason::UnsupportedType;
		}
		else {
			entry.Score = Score(candidate, frameIndex);
			entry.Reason = ShadowLightSelectionReason::BudgetExceeded;
		}

		result.Entries.push_back(entry);
	}

	std::vector<std::size_t> eligible;
	for (std::size_t i = 0; i < result.Entries.size(); ++i) {
		if (result.Entries[i].Reason == ShadowLightSelectionReason::BudgetExceeded) eligible.push_back(i);
	}

	std::sort(eligible.begin(), eligible.end(), [&](std::size_t left, std::size_t right) {
		const auto& a = result.Entries[left];
		const auto& b = result.Entries[right];
		if (a.Score != b.Score) return a.Score > b.Score;

		return a.StableId < b.StableId;
	});

	if (eligible.size() > m_xConfig.MaxSelectedRTLights) eligible.resize(m_xConfig.MaxSelectedRTLights);

	std::unordered_set<std::uint64_t> selectedIds;
	for (const auto index : eligible) {
		auto& entry = result.Entries[index];

		const auto history = m_mHistory.find(entry.StableId);
		entry.Selected = true;
		entry.Reason = history != m_mHistory.end() && history->second.WasSelected ? ShadowLightSelectionReason::HeldByHysteresis : ShadowLightSelectionReason::Selected;

		result.SelectedGlobalLightIndices.push_back(entry.GlobalLightIndex);

		selectedIds.insert(entry.StableId);
	}

	for (auto it = m_mHistory.begin(); it != m_mHistory.end(); ++it) {
		if (selectedIds.find(it->first) == selectedIds.end()) it->second.WasSelected = false;
	}

	for (const auto& entry : result.Entries) {
		if (!entry.Selected) continue;

		auto& history = m_mHistory[entry.StableId];
		if (!history.WasSelected) history.SelectedAtFrame = frameIndex;

		history.WasSelected = true;
	}
	return result;
}
