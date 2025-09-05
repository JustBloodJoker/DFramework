#pragma once

#include <pch.h>
#include <D3DFramework/GraphicUtilites/StructuredBuffer.h>
#include <D3DFramework/GraphicUtilites/DepthStencilView.h>
#include <System/CameraFrustum.h>

struct IndirectMeshRenderData {
	D3D12_GPU_VIRTUAL_ADDRESS CBMatrices;
	D3D12_GPU_VIRTUAL_ADDRESS CBMaterials;
	D3D12_GPU_VIRTUAL_ADDRESS SRVBones;
	D3D12_VERTEX_BUFFER_VIEW VertexBufferView;
	D3D12_INDEX_BUFFER_VIEW IndexBufferView;
	D3D12_DRAW_INDEXED_ARGUMENTS DrawArguments;
};

struct MeshAABBInstanceData {
	dx::XMFLOAT3 MaxP;
	UINT padd;
	dx::XMFLOAT3 MinP;
	UINT CommandIndex;
};

struct InputMeshesCullingProcessData {
	FD3DW::StructuredBuffer* InputCommandsBuffer;
	std::vector<MeshAABBInstanceData> Instances;
	ID3D12Device* Device;
	ID3D12GraphicsCommandList* CommandList;
	CameraFrustum CameraFrustum;
	FD3DW::DepthStencilView* DepthResource;
};

struct MeshesCullingCameraData {
	std::array<dx::XMFLOAT4, 6> CameraPlanes;
	dx::XMMATRIX ViewProjection;
	UINT InstancesCount;
	INT MipLevels;
	INT HiZWidth;
	INT HiZHeight;
};

struct IndirectRenderDataPair {
	IndirectMeshRenderData IndirectExecuteData;
	MeshAABBInstanceData AABBInstanceData;
};