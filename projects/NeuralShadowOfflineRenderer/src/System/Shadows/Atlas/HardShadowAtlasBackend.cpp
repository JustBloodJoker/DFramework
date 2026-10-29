#include "pch.h"

#include <System/Shadows/Atlas/HardShadowAtlasBackend.h>

#include <System/Shadows/Atlas/ShadowAtlasRenderer.h>

std::shared_ptr<FD3DW::ExecutionHandle> HardShadowAtlasBackend::Schedule(ShadowAtlasRenderer& coordinator,std::vector<std::shared_ptr<FD3DW::ExecutionHandle>> dependencies) {
	return coordinator.ScheduleHardShadowAtlas(std::move(dependencies));
}

HardShadowAtlasFilterSettings HardShadowAtlasBackend::GetFilterSettings() const {
	
	std::lock_guard<std::mutex> lock(m_xShadowParamsMutex);
	return m_xShadowParams.GetFilterSettings();
}

void HardShadowAtlasBackend::SetFilterSettings(const HardShadowAtlasFilterSettings& settings) {
	
	std::lock_guard<std::mutex> lock(m_xShadowParamsMutex);
	m_xShadowParams.SetFilterSettings(settings);
}
