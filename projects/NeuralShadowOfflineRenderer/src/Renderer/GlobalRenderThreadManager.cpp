#include "pch.h"

#include <Renderer/GlobalRenderThreadManager.h>
#include <Config/LightingConfig.h>
#include <Config/PipelineRegistry.h>
#include <Config/RendererLimits.h>
#include <Config/RenderFormats.h>
#include <Config/ShaderBindings.h>
#include <Config/ShadowConfig.h>
#include <Config/RendererResources.h>

GlobalRenderThreadManager::GlobalRenderThreadManager() {
	m_iMaxThreadsCount = NSOfflineConfig::RendererLimits::RenderWorkerCount;
}
