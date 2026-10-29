#pragma once

#include <System/Shadows/ShadowLightSelector.h>
#include <System/Shadows/RayTracing/RTGeometryCoverage.h>
#include <ShadowData/ShadowLightParameters.h>
#include <D3DFramework/GraphicUtilites/RenderThreadUtils/ExecutionHandle.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

class ShadowAtlasRenderer;

class PerLightRTShadowBackend {

	friend class ShadowAtlasRenderer;

public:
	std::shared_ptr<FD3DW::ExecutionHandle> Schedule(ShadowAtlasRenderer& coordinator, std::vector<std::shared_ptr<FD3DW::ExecutionHandle>> dependencies);
	NSRTQualitySettings GetQualitySettings() const;
	void SetQualitySettings(const NSRTQualitySettings& settings);

protected:
	ShadowLightSelector m_xLightSelector;
	std::atomic<std::uint64_t> m_uNextPerLightRTGeneration{ 1 };
	mutable std::mutex m_xQualitySettingsMutex;
	NSRTQualitySettings m_xQualitySettings;
};
