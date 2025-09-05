#pragma once

#include <pch.h>
#include <MainRenderer/MainRendererComponent.h>
#include <MainRenderer/GlobalRenderThreadManager.h>

class AudioSystem : public MainRendererComponent {
public:
	AudioSystem() = default;
	virtual ~AudioSystem() = default;

public:
	std::shared_ptr<FD3DW::ExecutionHandle> OnStartTick();

};