#pragma once

#include <pch.h>
#include <WinWindow/Utils/Reflection/Reflection.h>

class ComponentHolder;
class World;

class IComponent {
public:
	IComponent() = default;
	virtual ~IComponent() = default;

public:
    REFLECT_BODY(IComponent)
    BEGIN_REFLECT(IComponent)
        REFLECT_PROPERTY(m_sName)
        REFLECT_PROPERTY(m_bIsActive)
        REFLECT_PROPERTY(m_pOwner)
    END_REFLECT(IComponent)

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