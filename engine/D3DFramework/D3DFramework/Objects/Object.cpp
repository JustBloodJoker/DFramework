#include "../pch.h"
#include "Object.h"


namespace FD3DW
{



	Object::Object() : m_pMaterialManager(std::make_unique<MaterialsManager>())
	{
	}

	Object::~Object()
	{
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