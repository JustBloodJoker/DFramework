#pragma once
#include "../pch.h"

#include "../GraphicUtilites/BufferMananger.h"

#include "MaterialsMananger.h"



namespace FDW
{


	class Object
	{

	public:

		Object();
		virtual ~Object();

		D3D12_VERTEX_BUFFER_VIEW* GetVertexBufferView() const;
		D3D12_INDEX_BUFFER_VIEW* GetIndexBufferView() const;
		size_t GetObjectBuffersCount() const;
		std::tuple<size_t, size_t, size_t, size_t, size_t> GetObjectParameters(size_t index) const;

		MaterialsManager* GetMaterialMananger() const;
		size_t GetMaterialSize() const;

	protected:

		std::unique_ptr<MaterialsManager> matMananger;

		std::vector<std::tuple<size_t, size_t, size_t, size_t, size_t>> objectParameters;

		wrl::ComPtr<ID3D12Resource> vertexBuffer;
		std::unique_ptr<D3D12_VERTEX_BUFFER_VIEW> vertexBufferView;

		wrl::ComPtr<ID3D12Resource> indexBuffer;
		std::unique_ptr<D3D12_INDEX_BUFFER_VIEW> indexBufferView;



	private:

		


	};


}