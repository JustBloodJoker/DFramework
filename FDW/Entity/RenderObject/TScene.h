#pragma once

#include <pch.h>
#include <D3DFramework/Objects/Scene.h>
#include <Entity/RenderObject/TMesh.h>
#include <Component/RenderObject/AnimationComponent.h>
#include <D3DFramework/Objects/ObjectVertexIndexDataCreator.h>


class TScene : public TMesh {
public:
	TScene() = default;
	TScene(std::string path);
	virtual ~TScene() = default;


public:
	BEGIN_FIELD_REGISTRATION(TScene, TMesh)
		REGISTER_FIELD(m_sPath);
	END_FIELD_REGISTRATION();

public:
	bool IsHaveAnimation();
	bool IsFreeze();
	void Freeze(bool b);

	std::vector<std::string> GetAnimations();
	void Play(std::string animName);
	void Stop();
	bool IsPlaying();

public:
	virtual void BeforeRenderInitAfterCreation(ID3D12Device* device, ID3D12GraphicsCommandList* list) override;
	virtual void BeforeRenderInitAfterLoad(ID3D12Device* device, ID3D12GraphicsCommandList* list) override;

protected:
	void CallCreationScene(ID3D12Device* device, ID3D12GraphicsCommandList* list);
	AnimationComponent* GetAnimationComponent();

protected:
	std::string m_sPath;
	std::unique_ptr<FD3DW::ObjectVertexIndexDataCreator<FD3DW::VertexFrameWork>> m_pObjectVBV_IBV;
	std::unique_ptr<FD3DW::Scene> m_pScene;
	

};