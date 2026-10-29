#pragma once

#include <D3DFramework/GraphicUtilites/DepthStencilView.h>
#include <D3DFramework/GraphicUtilites/RenderTarget.h>
#include <Renderer/OfflineRendererComponent.h>
#include <Component/RenderObject/MeshesIndirectRenderData.h>
#include <Component/RenderObject/MeshComponent.h>
#include <System/Shadows/RayTracing/RTGeometryCoverage.h>

#include <string>

namespace FD3DW { class GraphicsPipelineObject; }

struct RenderMeshesSystemIndirectRenderData {
	std::vector<FD3DW::RenderTarget*> RTV;
	FD3DW::DepthStencilView* DSV;
	D3D12_CPU_DESCRIPTOR_HANDLE DSV_CPU;
	D3D12_CPU_DESCRIPTOR_HANDLE RTV_CPU;
	D3D12_RECT Rect;
	D3D12_VIEWPORT Viewport;
};


class RenderMeshesSystem : public OfflineRendererComponent {
public:
	RenderMeshesSystem() = default;
	virtual ~RenderMeshesSystem() = default;


public:
	virtual void AfterConstruction() override;
	virtual void ProcessNotify(NRenderSystemNotifyType type) override;

	std::shared_ptr<FD3DW::ExecutionHandle> OnStartRenderTick(std::shared_ptr<FD3DW::ExecutionHandle> handle);
	std::shared_ptr<FD3DW::ExecutionHandle> OnStartTLASCall(std::vector < std::shared_ptr<FD3DW::ExecutionHandle>> handle);
	std::shared_ptr<FD3DW::ExecutionHandle> IndirectRender(std::vector<std::shared_ptr<FD3DW::ExecutionHandle>> handle, RenderMeshesSystemIndirectRenderData data);

	FD3DW::AccelerationStructureBuffers GetTLAS() const;
	
	NSRTGeometryCoverageState GetRTGeometryCoverageState() const;
	NSRTGeometryAccounting GetRTGeometryAccounting() const;
	
	FD3DW::FResource* GetRTGeometryInstancesResource() const;
	
	ID3D12Resource* GetRTAlphaIndexBuffer() const;
	ID3D12Resource* GetRTAlphaVertexBuffer() const;
protected:
	
	void RefreshRTGeometryCoverageState();
	void RefreshRTGeometryBindings(ID3D12GraphicsCommandList* list);


	mutable std::mutex m_xRTGeometryCoverageMutex;
	
	NSRTGeometryCoverageState m_xRTGeometryCoverageState;
	NSRTGeometryAccounting m_xRTGeometryAccounting;
	
	std::vector<NSRTGeometrySupportClass> m_vRTGeometrySupportClasses;
	std::unique_ptr<FD3DW::StructuredBuffer> m_pRTGeometryInstances;
	
	ID3D12Resource* m_pRTAlphaIndexBuffer = nullptr;
	ID3D12Resource* m_pRTAlphaVertexBuffer = nullptr;

	std::atomic<bool> m_bNeedUpdateTLAS{ true };
	std::atomic<bool> m_bNeedUpdateMeshesActivationDeactivation{ true };

	std::vector< MeshComponent*> m_vActiveMeshComponents;

	FD3DW::AccelerationStructureBuffers m_xTLASBufferData;
	std::vector<IndirectMeshRenderData> m_vMeshRenderData;
	std::vector<UINT> m_vMeshSourceIds;
	
	wrl::ComPtr<ID3D12CommandSignature> m_pIndirectCommandSignature = nullptr;
	
	std::unique_ptr<FD3DW::StructuredBuffer> m_pIndirectExecuteCommandsBuffer = nullptr;
};
