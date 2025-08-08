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
	};

	class Object
	{

	public:

		Object();
		virtual ~Object();


		ID3D12Resource* GetVertexBuffer() const;
		ID3D12Resource* GetIndexBuffer() const;

		D3D12_VERTEX_BUFFER_VIEW* GetVertexBufferView() const;
		D3D12_INDEX_BUFFER_VIEW* GetIndexBufferView() const;
		size_t GetObjectBuffersCount() const;
		ObjectDesc GetObjectParameters(size_t index) const;

		MaterialsManager* GetMaterialMananger() const;
		size_t GetMaterialSize() const;
		
		virtual size_t GetVertexStructSize() const = 0;

	protected:

		std::unique_ptr<MaterialsManager> m_pMaterialManager;

		std::vector<ObjectDesc> m_vObjectParameters;

		wrl::ComPtr<ID3D12Resource> m_pVertexBuffer;
		std::unique_ptr<D3D12_VERTEX_BUFFER_VIEW> m_pVertexBufferView;

		wrl::ComPtr<ID3D12Resource> m_pIndexBuffer;
		std::unique_ptr<D3D12_INDEX_BUFFER_VIEW> m_pIndexBufferView;


	protected:  //RT BLAS

		


	};


}