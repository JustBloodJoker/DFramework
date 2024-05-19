#include "../pch.h"
#include "Object.h"


namespace FDW
{



	Object::Object() : matMananger(std::make_unique<MaterialsManager>())
	{
		ZeroMemory(&vertexBufferView, sizeof(vertexBufferView));
		ZeroMemory(&indexBufferView, sizeof(indexBufferView));
	}

	Object::~Object()
	{
	}

	D3D12_VERTEX_BUFFER_VIEW* Object::GetVertexBufferView() const
	{
		return vertexBufferView.get();
	}

	D3D12_INDEX_BUFFER_VIEW* Object::GetIndexBufferView() const
	{
		return indexBufferView.get();
	}

	size_t Object::GetObjectBuffersCount() const
	{
		return objectParameters.size();
	}

	std::tuple<size_t, size_t, size_t, size_t, size_t> Object::GetObjectParameters(size_t index) const
	{
		return index < objectParameters.size() ? objectParameters[index] : std::tuple<size_t, size_t, size_t, size_t, size_t>();
	}

	MaterialsManager* Object::GetMaterialMananger() const
	{
		return matMananger.get();
	}

	size_t Object::GetMaterialSize() const
	{
		return matMananger->GetMaterialSize();
	}

}