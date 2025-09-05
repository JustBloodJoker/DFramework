#pragma once

#include <pch.h>
#include <MainRenderer/MainRendererComponent.h>
#include <MainRenderer/GlobalRenderThreadManager.h>
#include <Component/Audio/AudioComponent.h>

class AudioSystem : public MainRendererComponent {
public:
	AudioSystem() = default;
	virtual ~AudioSystem() = default;

public:
	std::shared_ptr<FD3DW::ExecutionHandle> OnStartTick(std::shared_ptr<FD3DW::ExecutionHandle> h);
	virtual void ProcessNotify(NRenderSystemNotifyType type) override;

protected:
	std::vector< AudioComponent*> m_vActiveAudio;
	std::atomic<bool> m_bIsNeedUpdateAudioActivation{ false };
};