#pragma once

#include <pch.h>
#include <RenderableObjects/IndirectMeshRenderableData.h>
#include <RenderableObjects/MeshMatricesStructure.h>
#include <RenderableObjects/MeshMaterialStructure.h>


struct InstanceData {
	dx::XMFLOAT3 CenterWS;
	float RadiusWS;
	UINT DrawIndex;
	UINT Pad[3];
};

class IndirectExecutionMeshObject {
public:
	IndirectExecutionMeshObject() = default;
	virtual ~IndirectExecutionMeshObject() = default;

	virtual bool IsCanBeIndirectExecuted();
	virtual std::vector<std::pair<IndirectMeshRenderableData, InstanceData>> GetDataToExecute() = 0;
};