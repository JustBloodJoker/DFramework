#pragma once
#include "../pch.h"

#include "../GraphicUtilites/BufferManager.h"

#include "MaterialsManager.h"



namespace FD3DW
{
	struct ObjectDesc
	{
		size_t VerticesCount;
		size_t VerticesOffset;
		size_t IndicesCount;
		size_t IndicesOffset;
		size_t MaterialIndex;
	};

	class Object
	{

	public:

		Object();
		virtual ~Object();

		D3D12_VERTEX_BUFFER_VIEW* GetVertexBufferView() const;
		D3D12_INDEX_BUFFER_VIEW* GetIndexBufferView() const;
		size_t GetObjectBuffersCount() const;
		ObjectDesc GetObjectParameters(size_t index) const;

		MaterialsManager* GetMaterialMananger() const;
		size_t GetMaterialSize() const;

		
	protected:

		std::unique_ptr<MaterialsManager> m_pMaterialManager;

		std::vector<ObjectDesc> m_vObjectParameters;

		wrl::ComPtr<ID3D12Resource> m_pVertexBuffer;
		std::unique_ptr<D3D12_VERTEX_BUFFER_VIEW> m_pVertexBufferView;

		wrl::ComPtr<ID3D12Resource> m_pIndexBuffer;
		std::unique_ptr<D3D12_INDEX_BUFFER_VIEW> m_pIndexBufferView;


	private:

		


	};


}