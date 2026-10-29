#include "pch.h"

#include <MainRenderer/GlobalRenderThreadManager.h>
#include <Config/RendererLimits.h>

GlobalRenderThreadManager::GlobalRenderThreadManager() {
	m_iMaxThreadsCount = FDWConfig::RendererLimits::RenderWorkerCount;
}
