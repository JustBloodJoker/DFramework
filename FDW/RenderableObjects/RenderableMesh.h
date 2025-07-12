#pragma once 

#include <pch.h>
#include <D3DFramework/Objects/Scene.h>
#include <RenderableObjects/BaseRenderableObject.h>
#include <RenderableObjects/RenderableMeshElement.h>
#include <D3DFramework/GraphicUtilites/ResourcePacker.h>

class RenderableMesh : public BaseRenderableObject {
public:
	RenderableMesh(std::unique_ptr<FD3DW::Scene> scene);
	virtual ~RenderableMesh() = default;

public:

	virtual void Init(ID3D12Device* device, ID3D12GraphicsCommandList* list) override;
	virtual void BeforeRender(const BeforeRenderInputData& data) override;
	virtual void Render(ID3D12GraphicsCommandList* list) override;

	std::vector<std::string> GetAnimations();
	void PlayAnimation(std::string animName);
	void StopAnimation();
	void FreezeAnimation(bool isFreezed);

	std::vector<RenderableMeshElement*> GetRenderableElements();

private:
	void AnimationTickUpdate(const BeforeRenderInputData& data);

private:
	std::vector<std::unique_ptr<FD3DW::SRVPacker>> m_vSRVPacks;
	std::unique_ptr<FD3DW::FResource> m_pStructureBufferBones;
	
private:
	std::unique_ptr<FD3DW::Scene> m_pScene = nullptr;
	std::vector<std::unique_ptr<RenderableMeshElement>> m_vRenderableElements;

	float m_fAnimationTime = 0.0;
	std::string m_sCurrentAnimation = "";
	bool m_bNeedResetBonesBuffer = false;
	bool m_bNeedFreezeBonesBuffer = false;
};