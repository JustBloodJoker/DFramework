#pragma once

#include <Component/Core/IComponent.h>
#include <D3DFramework/GraphicUtilites/StructuredBuffer.h>
#include <D3DFramework/Objects/Scene.h>
#include <D3DFramework/Objects/ObjectVertexIndexDataCreator.h>

struct AnimationComponentInputData {
	ID3D12Device* Device;
	ID3D12GraphicsCommandList* CommandList;
	float DT;
};

struct AnimationComponentGPUConstantBufferData {
	UINT VertexCount;
	UINT NumBones;
	UINT AnimationEnabled;
	UINT _pad3;
};

class AnimationComponent : public IComponent {
public:
	AnimationComponent();
	virtual ~AnimationComponent() = default;

public:
    REFLECT_BODY(AnimationComponent)
    BEGIN_REFLECT(AnimationComponent, IComponent)
        REFLECT_PROPERTY(m_fAnimationTime)
        REFLECT_PROPERTY(m_sCurrentAnimation)
        REFLECT_PROPERTY(m_bNeedFreezeBonesBuffer)
        REFLECT_PROPERTY(m_bIsBonesActive)
    END_REFLECT(AnimationComponent)


public:
	virtual void Init() override;
	virtual void Destroy() override;
	virtual void Activate(bool b) override;

	virtual bool IsFreeze();
	virtual void Freeze(bool b);

	std::vector<std::string> GetAnimations();
	virtual void Play(std::string animName);
	virtual void Stop();
	virtual bool IsPlaying();
	
	virtual void SetScene(ID3D12Device* device, ID3D12GraphicsCommandList* list, FD3DW::Scene* scene, FD3DW::IObjectVertexIndexDataCreator* sceneRenderData);
	virtual FD3DW::FResource* GetResource();

	virtual void OnAnimationUpdateTick(const AnimationComponentInputData& data);
	virtual void ProcessGPUSkinning(ID3D12GraphicsCommandList* computeList);

	std::weak_ptr<bool> IsBonesActive();

protected:
	void EnableAnimation(bool b);

protected:
	FD3DW::Scene* m_pScene = nullptr;
	FD3DW::IObjectVertexIndexDataCreator* m_pSceneCreatorData = nullptr;
	std::unique_ptr<FD3DW::FResource> m_pStructureBufferBones = nullptr;
	std::unique_ptr<FD3DW::StructuredBuffer> m_pStructuredSceneVertexBuffer = nullptr;
	std::unique_ptr<FD3DW::UploadBuffer< AnimationComponentGPUConstantBufferData>> m_pGPUSkinningConstantBuffer = nullptr;

	std::shared_ptr<bool> m_bIsBonesActive;
	float m_fAnimationTime = 0.0;
	std::string m_sCurrentAnimation = "";
	bool m_bNeedResetBonesBuffer = false;
	bool m_bNeedFreezeBonesBuffer = false;
	std::atomic<bool> m_bCallGPUCulling{ false };

	AnimationComponentGPUConstantBufferData m_xData;
};