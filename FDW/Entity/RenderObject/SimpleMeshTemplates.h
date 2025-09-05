#pragma once
#include <pch.h>

#include <Entity/RenderObject/TSimpleMesh.h>
#include <Entity/RenderObject/SimpleMeshTemplatesGenerators.h>

#define DECL_SIMPLE_MESH_ENTITY(name) class TSimple##name : public TSimpleMesh {																								\
public:																																									\
	TSimple##name() = default;																																			\
	virtual ~TSimple##name() = default;																																	\
	virtual void AfterCreation() override { m_sName = std::string("Simple") + std::string(#name); };																	\
	BEGIN_FIELD_REGISTRATION(TSimple##name, TSimpleMesh)																												\
		END_FIELD_REGISTRATION();																																		\
	virtual std::unique_ptr<FD3DW::SimpleObject<FD3DW::SceneVertexFrameWork>> CreateSimpleObject(ID3D12Device* device, ID3D12GraphicsCommandList* list) override {		\
		return std::make_unique<FD3DW::SimpleObject<FD3DW::SceneVertexFrameWork>>(device, list, GenerateSimple##name##Scene);											\
	}																																									\
};

DECL_SIMPLE_MESH_ENTITY(Plane);
DECL_SIMPLE_MESH_ENTITY(Cube);
DECL_SIMPLE_MESH_ENTITY(Cone);
DECL_SIMPLE_MESH_ENTITY(Sphere);