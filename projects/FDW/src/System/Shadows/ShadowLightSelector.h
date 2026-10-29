#pragma once

enum class ShadowLightSelectionReason : std::uint8_t {
	Selected,
	HeldByHysteresis,
	BudgetExceeded,
	ShadowsDisabled,
	UnsupportedType
};

struct ShadowLightCandidate {
	std::uint64_t StableId = 0;
	std::uint32_t GlobalLightIndex = 0;
	std::uint32_t LightType = 0;
	bool CastsShadows = true;
	float ShadowPriority = 0.0f;
	float ScreenInfluence = 0.0f;
	float Intensity = 0.0f;
	float Range = 0.0f;
	float Distance = 0.0f;
};

struct ShadowLightSelectionEntry {
	std::uint64_t StableId = 0;
	std::uint32_t GlobalLightIndex = 0;
	float Score = 0.0f;
	bool Selected = false;
	ShadowLightSelectionReason Reason = ShadowLightSelectionReason::BudgetExceeded;
};

struct ShadowLightSelectionResult {
	std::vector<std::uint32_t> SelectedGlobalLightIndices;
	std::vector<ShadowLightSelectionEntry> Entries;
};

struct ShadowLightSelectorConfig {
	std::uint32_t MaxSelectedRTLights = 8;
	std::uint64_t MinimumHoldFrames = 8;
	float HysteresisBonus = 0.25f;
};

class ShadowLightSelector {

public:
	explicit ShadowLightSelector(ShadowLightSelectorConfig config = {});
	ShadowLightSelectionResult Select(const std::vector<ShadowLightCandidate>& candidates, std::uint64_t frameIndex);
	const ShadowLightSelectorConfig& GetConfig() const noexcept;

protected:
	struct HistoryEntry {
		std::uint64_t SelectedAtFrame = 0;
		bool WasSelected = false;
	};

protected:
	float Score(const ShadowLightCandidate& candidate, std::uint64_t frameIndex) const;
	static bool IsRayTracedLightTypeSupported(std::uint32_t lightType) noexcept;

protected:
	ShadowLightSelectorConfig m_xConfig;
	std::unordered_map<std::uint64_t, HistoryEntry> m_mHistory;
};
