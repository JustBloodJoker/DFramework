#pragma once

#include <pch.h>
#include <D3DFramework/GraphicUtilites/StructuredBuffer.h>
#include <D3DFramework/GraphicUtilites/ResourcePacker.h>
#include <D3DFramework/GraphicUtilites/DepthStencilView.h>
#include <RenderableObjects/ObjectsCulling/InstanceData.h>
#include <RenderableObjects/ObjectsCulling/CullingCameraStructure.h>
#include <RenderableObjects/ObjectsCulling/BVHBuilder.h>
#include <Camera/CameraFrustum.h>

struct InputObjectCullingProcessData {
	FD3DW::StructuredBuffer* InputCommandsBuffer;
	std::vector<InstanceData> Instances;
	ID3D12Device* Device;
	ID3D12GraphicsCommandList* CommandList;
	CameraFrustum CameraFrustum;
	FD3DW::DepthStencilView* DepthResource;
};


class ObjectCulling {
public:
	ObjectCulling(ID3D12Device* device);
	~ObjectCulling() = default;
	
	bool CheckFrustumCulling(CameraFrustum fr ,const InstanceData& data);
	void ProcessGPUCulling(const InputObjectCullingProcessData& data);

	FD3DW::StructuredBuffer* GetResultBuffer();
	UINT CountBufferOffset(UINT count);

private:
	void Init(ID3D12Device* device);
	void RecreateOutputCommandBuffer(ID3D12Device* device, ID3D12GraphicsCommandList* list, UINT count);
	void LoadDataToInstancesBuffer(ID3D12Device* device, ID3D12GraphicsCommandList* list, std::vector<InstanceData> data);
	void LoadDataToCameraBuffer(CameraFrustum frustum, UINT instancesCount, FD3DW::DepthStencilView* resource);
	FD3DW::UAVResourceDesc GetDefaultDescriptor(UINT commandsNum);

private:
	std::unique_ptr<FD3DW::UploadBuffer<CullingCameraStructure>> m_pCullingCameraBuffer;
	std::unique_ptr<FD3DW::StructuredBuffer> m_pInstancesDataBuffer;

	std::unique_ptr<FD3DW::StructuredBuffer> m_pOutputCommandsBuffer;
	std::unique_ptr<FD3DW::SRV_UAVPacker> m_pPack;

};