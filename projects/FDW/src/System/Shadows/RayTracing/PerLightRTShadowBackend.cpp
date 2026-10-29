#include "pch.h"

#include <System/Shadows/RayTracing/PerLightRTShadowBackend.h>

#include <System/Shadows/Atlas/ShadowAtlasRenderer.h>

std::shared_ptr<FD3DW::ExecutionHandle> PerLightRTShadowBackend::Schedule(ShadowAtlasRenderer& coordinator, std::vector<std::shared_ptr<FD3DW::ExecutionHandle>> dependencies) {
	return coordinator.SchedulePerLightRTVisibility( std::move(dependencies) );
}

NSRTQualitySettings PerLightRTShadowBackend::GetQualitySettings() const {
	std::lock_guard<std::mutex> lock(m_xQualitySettingsMutex);
	return m_xQualitySettings;
}

void PerLightRTShadowBackend::SetQualitySettings(const NSRTQualitySettings& settings) {
	std::lock_guard<std::mutex> lock(m_xQualitySettingsMutex);
	m_xQualitySettings = ShadowLightParameters::SanitizeQualitySettings(settings);
}
