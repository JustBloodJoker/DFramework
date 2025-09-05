#pragma once

#include <pch.h>
#include <MainRenderer/MainRendererComponent.h>
#include <System/MeshesCullingSubSystem.h>
#include <Entity/RenderObject/MeshesIndirectRenderData.h>
#include <Entity/RenderObject/MeshComponent.h>

enum class MeshCullingType {
	None = 0,
	GPU = 1,
};

class RenderMeshesSystem : public MainRendererComponent {
public:
	RenderMeshesSystem() = default;
	virtual ~RenderMeshesSystem() = default;


public:
	virtual void AfterConstruction() override;
	virtual void ProcessNotify(NRenderSystemNotifyType type) override;
	
	std::shared_ptr<FD3DW::ExecutionHandle> OnStartRenderTick(std::shared_ptr<FD3DW::ExecutionHandle> handle);
	std::shared_ptr<FD3DW::ExecutionHandle> OnStartTLASCall(std::shared_ptr<FD3DW::ExecutionHandle> handle);
	std::shared_ptr<FD3DW::ExecutionHandle> UpdateHiZResource(std::shared_ptr<FD3DW::ExecutionHandle> handle);
	std::shared_ptr<FD3DW::ExecutionHandle> PreDepthRender(std::shared_ptr<FD3DW::ExecutionHandle> handle);
	std::shared_ptr<FD3DW::ExecutionHandle> IndirectRender(std::shared_ptr<FD3DW::ExecutionHandle> handle);

protected:
	std::atomic<bool> m_bNeedUpdateTLAS{ true };
	std::atomic<bool> m_bNeedUpdateMeshesActivationDeactivation{ true };

	MeshCullingType m_xMeshCullingType = MeshCullingType::None;

	std::vector< MeshComponent*> m_vActiveMeshComponents;

	FD3DW::AccelerationStructureBuffers m_xTLASBufferData;
	std::unique_ptr<MeshesCullingSubSystem> m_pMeshesCulling = nullptr;


	std::vector<IndirectMeshRenderData> m_vMeshRenderData;
	std::vector<MeshAABBInstanceData> m_vMeshAABBInstanceData;

	wrl::ComPtr<ID3D12CommandSignature> m_pIndirectCommandSignature = nullptr;
	std::unique_ptr<FD3DW::StructuredBuffer> m_pIndirectExecuteCommandsBuffer = nullptr;
};
