#pragma once

#include <ShadowData/ShadowLightParameters.h>
#include <Config/ShadowConfig.h>
#include <System/RectsPacker.h>

#include <vector>

class ShadowAtlasRenderer;

class ShadowAtlasAllocator {

	friend class ShadowAtlasRenderer;

protected:

	std::vector<LightAtlasMeta> m_vMetas;
	std::vector<FDWConfig::Shadows::AtlasLightIndex> m_vTexelMap;
	std::vector<Rect> m_vDirtyRegions;

	RectsPacker m_xAtlasPacker{
		static_cast<int>(FDWConfig::Shadows::AtlasWidth),
		static_cast<int>(FDWConfig::Shadows::AtlasHeight),
		1
	};
};
