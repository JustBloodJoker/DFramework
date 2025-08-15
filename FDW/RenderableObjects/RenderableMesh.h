#pragma once 

#include <pch.h>
#include <D3DFramework/Objects/Scene.h>
#include <RenderableObjects/BaseRenderableObject.h>
#include <RenderableObjects/IndirectExecutionMeshObject.h>
#include <RenderableObjects/RenderableMeshElement.h>
#include <D3DFramework/GraphicUtilites/ResourcePacker.h>

class RenderableMesh : virtual public BaseRenderableObject, virtual public IndirectExecutionMeshObject {
public:
	RenderableMesh() = default;
	RenderableMesh(std::string path);
	virtual ~RenderableMesh() = default;

public:

	virtual void Init(ID3D12Device* device, ID3D12GraphicsCommandList* list) override;
	virtual void BeforeRender(const BeforeRenderInputData& data) override;
	virtual void DeferredRender(ID3D12GraphicsCommandList* list) override;
	virtual void ForwardRender(ID3D12GraphicsCommandList* list) override;
	virtual RenderPass GetRenderPass() const override;


	virtual bool IsCanBeIndirectExecuted() override;
	virtual std::vector< IndirectMeshRenderableData> GetDataToExecute() override;

	std::vector<std::string> GetAnimations();
	void PlayAnimation(std::string animName);
	void StopAnimation();
	void FreezeAnimation(bool isFreezed);

	std::vector<RenderableMeshElement*> GetRenderableElements();

	virtual void InitBLASBuffers(ID3D12Device5* device, ID3D12GraphicsCommandList4* list) override;
	virtual std::vector<std::pair<FD3DW::AccelerationStructureBuffers, dx::XMMATRIX>> GetBLASInstances() override;


	virtual bool IsNeedUpdateTLAS() override;
	virtual void AfterTLASUpdate() override;

public:
	BEGIN_FIELD_REGISTRATION(RenderableMesh, BaseRenderableObject)
		REGISTER_FIELD(m_sPath);
		REGISTER_FIELD(m_vRenderableElements);
	END_FIELD_REGISTRATION();

private:
	void RenderObjectsInPass(RenderPass pass, ID3D12GraphicsCommandList* list);

private:
	void AnimationTickUpdate(const BeforeRenderInputData& data);

private:
	std::unique_ptr<FD3DW::FResource> m_pStructureBufferBones;
	
private:
	std::unique_ptr<FD3DW::Scene> m_pScene = nullptr;
	std::vector<std::unique_ptr<RenderableMeshElement>> m_vRenderableElements;

	float m_fAnimationTime = 0.0;
	std::string m_sCurrentAnimation = "";
	bool m_bNeedResetBonesBuffer = false;
	bool m_bNeedFreezeBonesBuffer = false;

	std::string m_sPath;
};