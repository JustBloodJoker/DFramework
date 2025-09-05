#pragma once

#include <pch.h>
#include <MainRenderer/MainRendererComponent.h>
#include <MainRenderer/GlobalRenderThreadManager.h>
#include <Entity/RenderObject/AnimationComponent.h>

class SceneAnimationSystem : public MainRendererComponent {
public:
	SceneAnimationSystem() = default;
	virtual ~SceneAnimationSystem() = default;

public:

	virtual void ProcessNotify(NRenderSystemNotifyType type) override;
	virtual std::shared_ptr<FD3DW::ExecutionHandle> OnStartRenderTick(std::shared_ptr<FD3DW::ExecutionHandle> sync);

protected:
	std::atomic<bool> m_bIsNeedUpdateActiveAnimations{ true };
	std::vector<AnimationComponent*> m_vActiveAnimations;

};