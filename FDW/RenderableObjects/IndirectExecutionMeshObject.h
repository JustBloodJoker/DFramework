#pragma once

#include <pch.h>
#include <RenderableObjects/IndirectMeshRenderableData.h>
#include <RenderableObjects/MeshMatricesStructure.h>
#include <RenderableObjects/MeshMaterialStructure.h>
#include <RenderableObjects/ObjectsCulling/InstanceData.h>



class IndirectExecutionMeshObject {
public:
	IndirectExecutionMeshObject() = default;
	virtual ~IndirectExecutionMeshObject() = default;

	virtual bool IsCanBeIndirectExecuted();
	virtual std::vector<std::pair<IndirectMeshRenderableData, InstanceData>> GetDataToExecute() = 0;
};