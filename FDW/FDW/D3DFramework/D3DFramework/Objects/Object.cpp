#include "../pch.h"
#include "Object.h"


namespace FD3DW
{



	Object::Object() : m_pMaterialManager(std::make_unique<MaterialsManager>())
	{
		ZeroMemory(&m_pVertexBufferView, sizeof(m_pVertexBufferView));
		ZeroMemory(&m_pIndexBufferView, sizeof(m_pIndexBufferView));
	}

	Object::~Object()
	{
	}

	D3D12_VERTEX_BUFFER_VIEW* Object::GetVertexBufferView() const
	{
		return m_pVertexBufferView.get();
	}

	D3D12_INDEX_BUFFER_VIEW* Object::GetIndexBufferView() const
	{
		return m_pIndexBufferView.get();
	}

	size_t Object::GetObjectBuffersCount() const
	{
		return m_vObjectParameters.size();
	}

	ObjectDesc Object::GetObjectParameters(size_t index) const
	{
		return index < m_vObjectParameters.size() ? m_vObjectParameters[index] : ObjectDesc();
	}

	MaterialsManager* Object::GetMaterialMananger() const
	{
		return m_pMaterialManager.get();
	}

	size_t Object::GetMaterialSize() const
	{
		return m_pMaterialManager->GetMaterialSize();
	}

}