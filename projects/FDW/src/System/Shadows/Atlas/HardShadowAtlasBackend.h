#pragma once

#include <ShadowData/ShadowLightParameters.h>
#include <D3DFramework/GraphicUtilites/RenderThreadUtils/ExecutionHandle.h>

#include <memory>
#include <mutex>
#include <vector>

class ShadowAtlasRenderer;

class HardShadowAtlasBackend {

	friend class ShadowAtlasRenderer;

public:
	std::shared_ptr<FD3DW::ExecutionHandle> Schedule(ShadowAtlasRenderer& coordinator, std::vector<std::shared_ptr<FD3DW::ExecutionHandle>> dependencies);
	HardShadowAtlasFilterSettings GetFilterSettings() const;
	void SetFilterSettings(const HardShadowAtlasFilterSettings& settings);

protected:
	AtlasRTShadowParams m_xShadowParams;
	mutable std::mutex m_xShadowParamsMutex;
};
