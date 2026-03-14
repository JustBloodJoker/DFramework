#pragma once
#include "../pch.h"

#include "../GraphicUtilites/BufferManager.h"

#include "MaterialsManager.h"



namespace FD3DW
{
	struct ObjectDesc
	{
		UINT VerticesCount;
		UINT VerticesOffset;
		UINT IndicesCount;
		UINT IndicesOffset;
		UINT MaterialIndex;
		dx::XMFLOAT3 ObjectMin;
		dx::XMFLOAT3 ObjectMax;
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