#pragma once

#include <Component/Core/IComponent.h>
#include <D3DFramework/GraphicUtilites/FResource.h>
#include <D3DFramework/Objects/Scene.h>

struct AnimationComponentInputData {
	ID3D12Device* Device;
	ID3D12GraphicsCommandList* CommandList;
	float DT;
};


class AnimationComponent : public IComponent {
public:
	AnimationComponent() = default;
	virtual ~AnimationComponent() = default;

public:
	BEGIN_FIELD_REGISTRATION(AnimationComponent, IComponent)
		REGISTER_FIELD(m_fAnimationTime)
		REGISTER_FIELD(m_sCurrentAnimation)
		REGISTER_FIELD(m_bNeedFreezeBonesBuffer)
	END_FIELD_REGISTRATION();


public:
	virtual void Init() override;
	virtual void Destroy() override;
	virtual void Activate(bool b) override;

	virtual bool IsFreeze();
	virtual void Freeze(bool b);

	virtual void Play(std::string animName);
	virtual void Stop();
	virtual bool IsPlaying();
	
	virtual void SetScene(ID3D12Device* device, FD3DW::Scene* scene);
	virtual FD3DW::FResource* GetResource();

	virtual void OnAnimationUpdateTick(const AnimationComponentInputData& data);

protected:
	FD3DW::Scene* m_pScene;
	std::unique_ptr<FD3DW::FResource> m_pStructureBufferBones;

	float m_fAnimationTime = 0.0;
	std::string m_sCurrentAnimation = "";
	bool m_bNeedResetBonesBuffer = false;
	bool m_bNeedFreezeBonesBuffer = false;
};