#pragma once

#include <pch.h>
#include <D3DFramework/Utilites/Serializer/ReflectionImpl.h>

class ComponentHolder;
class World;

class IComponent {
public:
	IComponent() = default;
	virtual ~IComponent() = default;

public:
	BEGIN_FIELD_REGISTRATION(IComponent)
		REGISTER_FIELD(m_sName);
		REGISTER_FIELD(m_bIsActive);
		REGISTER_FIELD(m_pOwner);
	END_FIELD_REGISTRATION();

public:
	void SetOwner(ComponentHolder* owner);
	ComponentHolder* GetOwner();
	World* GetWorld();

	std::string GetName() const;
	void SetName(std::string name);

public:
	virtual void Init();
	virtual void Destroy() = 0;

public:
	virtual bool IsActive();
	virtual void Activate(bool a);

protected:
	bool m_bIsActive = true;
	std::string m_sName = "Component";
	ComponentHolder* m_pOwner = nullptr;
};