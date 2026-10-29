#pragma once

#include <WinWindow/Utils/Reflection/Reflection.h>
#include <Component/Core/IComponent.h>
#include <System/NRenderSystemNotifyType.h>

class World;

class ComponentHolder {
public:
	ComponentHolder() = default;
	virtual ~ComponentHolder() = default;

public:
    template<typename T, typename... Args>
    T* AddComponent(Args&&... args) {
        auto comp = std::make_shared<T>(std::forward<Args>(args)...);
        comp->SetOwner(this);
        comp->Init();
        m_vComponents.push_back(comp);
        return comp.get();
    }

    bool IsActive();
    void Activate(bool b);

    template<typename T>
    T* GetComponent() {
        for (auto& comp : m_vComponents) {
            if (auto casted = dynamic_cast<T*>(comp.get())) return casted;
        }
        return nullptr;
    }

    template<typename T>
    std::vector<T*> GetComponents() {
        std::vector<T*> result;
        for (auto& comp : m_vComponents) {
            if (auto casted = dynamic_cast<T*>(comp.get())) {
                result.push_back(casted);
            }
        }
        return result;
    }

    template<typename T, typename Func>
    void ForEachComponentOfType(Func&& func) {
        for (auto& comp : m_vComponents) {
            if (auto casted = dynamic_cast<T*>(comp.get())) {
                func(casted);
            }
        }
    }

public:
    void SetWorld(World* world);
    World* GetWorld();

public:
    virtual void AfterCreation();
    virtual void Init();

    void AddNotifyToPull(NRenderSystemNotifyType type);

public:

    REFLECT_BODY(ComponentHolder)
    BEGIN_REFLECT(ComponentHolder)
        REFLECT_PROPERTY(m_vComponents)
        REFLECT_PROPERTY(m_pWorld)
        REFLECT_PROPERTY(m_sName)
    END_REFLECT(ComponentHolder)

protected:
    std::string m_sName = "Entity";
    World* m_pWorld = nullptr;
	std::vector<std::shared_ptr<IComponent>> m_vComponents;
};

// IComponent's reflected owner pointer requires ComponentHolder to be complete.
// Keeping the registrar here makes IComponent and derived component headers
// independently includable without changing the serialized property set.
BEGIN_REFLECT(IComponent)
    REFLECT_PROPERTY(m_sName)
    REFLECT_PROPERTY(m_bIsActive)
    REFLECT_PROPERTY(m_pOwner)
END_REFLECT(IComponent)
