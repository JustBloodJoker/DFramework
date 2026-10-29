#pragma once
#include "../pch.h"

#include "../GraphicUtilites/BufferManager.h"

#include "MaterialsManager.h"



namespace FD3DW
{
	enum class MaterialAlphaMode : UINT
	{
		Opaque = 0u,
		Mask = 1u,
		Blend = 2u,
		Unknown = 3u
	};

	struct ObjectDesc
	{
		UINT VerticesCount;
		UINT VerticesOffset;
		UINT IndicesCount;
		UINT IndicesOffset;
		UINT MaterialIndex;
		dx::XMFLOAT3 ObjectMin;
		dx::XMFLOAT3 ObjectMax;
		MaterialAlphaMode AlphaMode = MaterialAlphaMode::Opaque;
		float AlphaCutoff = 0.5f;
		UINT DoubleSided = 0u;
		UINT PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		UINT HasNormals = 0u;
		UINT HasTangents = 0u;
		UINT HasTextureCoordinates = 0u;
		UINT HasBones = 0u;
		std::string MeshName;
	};

	class Object
	{

	public:

		Object();
		virtual ~Object();

		size_t GetObjectBuffersCount() const;
		ObjectDesc GetObjectParameters(size_t index) const;

		MaterialsManager* GetMaterialMananger() const;
		size_t GetMaterialSize() const;
		
	protected:

		std::unique_ptr<MaterialsManager> m_pMaterialManager;

		std::vector<ObjectDesc> m_vObjectParameters;
	};


}
