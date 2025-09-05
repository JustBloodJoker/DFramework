#pragma once

#include "../pch.h"
#include "Object.h"

namespace FD3DW {

	struct AccelerationStructureInput {
		ID3D12Resource* vertexBuffer;
		UINT64 vertexOffset;
		UINT vertexCount;
		UINT vertexStride;
		DXGI_FORMAT vertexFormat;

		ID3D12Resource* indexBuffer;
		UINT64 indexOffset;
		UINT indexCount;
		UINT indexStride;
		DXGI_FORMAT indexFormat;

		UINT HitGroupIndex;

		D3D12_RAYTRACING_GEOMETRY_FLAGS flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
	};

	struct AccelerationStructureBuffers {
		wrl::ComPtr<ID3D12Resource> pScratch;
		wrl::ComPtr<ID3D12Resource> pResult;
		wrl::ComPtr<ID3D12Resource> pInstanceDesc;
		UINT HitGroupIndex;
	};


	struct RTObjectData {
		AccelerationStructureBuffers TLASBuffers;
		std::vector<AccelerationStructureBuffers> BLASBuffers;
		std::vector<dx::XMMATRIX> Transforms;

		std::vector<std::pair<AccelerationStructureBuffers, dx::XMMATRIX>> GetInstances() const;
		void UpdateTLAS(ID3D12Device5* device, ID3D12GraphicsCommandList4* list);
	};


	std::vector<AccelerationStructureBuffers> CreateBottomLevelAS(ID3D12Device5* device,ID3D12GraphicsCommandList4* cmdList,const std::vector<AccelerationStructureInput>& geometries, UINT hitGroupIndex, bool isStandaloneGeometries);
	AccelerationStructureBuffers CreateTopLevelAS(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, const std::vector<std::pair<AccelerationStructureBuffers, dx::XMMATRIX>>& instances);
	AccelerationStructureBuffers CreateTopLevelAS(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, std::vector<AccelerationStructureBuffers> buffers, dx::XMMATRIX defaultMatrix = dx::XMMatrixIdentity());
	void UpdateTopLevelAS(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, AccelerationStructureBuffers& tlasBuffers, const std::vector<std::pair<AccelerationStructureBuffers, dx::XMMATRIX>>& instances);
	
	std::vector<AccelerationStructureInput> CreateGeometriesForObject(Object* obj);
	AccelerationStructureInput CreateGeometryForObject(Object* obj, UINT objDataIdx);
	std::vector<AccelerationStructureBuffers> CreateBLASForObject(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, Object* obj, UINT hitGroupIndex, bool isStandaloneGeometries);
	AccelerationStructureBuffers CreateBLASForObjectInIndex(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, Object* obj, UINT objectParamsIndex, UINT hitGroupIndex);

	RTObjectData CreateRTDataForObject(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, UINT hitGroupIndex, Object* obj);
}
