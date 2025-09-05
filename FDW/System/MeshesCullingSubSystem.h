#pragma once

#include <pch.h>
#include <D3DFramework/GraphicUtilites/StructuredBuffer.h>
#include <D3DFramework/GraphicUtilites/ResourcePacker.h>
#include <D3DFramework/GraphicUtilites/DepthStencilView.h>
#include <Entity/RenderObject/MeshesIndirectRenderData.h>
#include <System/CameraFrustum.h>


class MeshesCullingSubSystem {
public:
	MeshesCullingSubSystem(ID3D12Device* device);
	~MeshesCullingSubSystem() = default;

	bool CheckFrustumCulling(CameraFrustum fr, const MeshAABBInstanceData& data);
	void ProcessGPUCulling(const InputMeshesCullingProcessData& data);

	void UpdateHiZResource(FD3DW::DepthStencilView* mainDSV, ID3D12Device* device, ID3D12GraphicsCommandList* list);

	FD3DW::StructuredBuffer* GetResultBuffer();
	UINT CountBufferOffset(UINT count);

private:
	void Init(ID3D12Device* device);
	void RecreateOutputCommandBuffer(ID3D12Device* device, ID3D12GraphicsCommandList* list, UINT count);
	void LoadDataToInstancesBuffer(ID3D12Device* device, ID3D12GraphicsCommandList* list, std::vector<MeshAABBInstanceData> data);
	void LoadDataToCameraBuffer(CameraFrustum frustum, UINT instancesCount);
	FD3DW::UAVResourceDesc GetDefaultDescriptor(UINT commandsNum);

private:
	std::unique_ptr<FD3DW::UploadBuffer<MeshesCullingCameraData>> m_pCullingCameraBuffer;
	std::unique_ptr<FD3DW::StructuredBuffer> m_pInstancesDataBuffer;

	std::unique_ptr<FD3DW::StructuredBuffer> m_pOutputCommandsBuffer;
	std::unique_ptr<FD3DW::SRV_UAVPacker> m_pPack;

	FD3DW::DepthStencilView* m_pLastDepthBufferResource = nullptr;
	std::unique_ptr<FD3DW::FResource> m_pHiZResource;

	bool m_bIsCanDoHiZOcclusion = false;

};