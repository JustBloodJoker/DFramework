#pragma once

#include <pch.h>
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

    void RemoveComponent(IComponent* ptr);
    void RemoveComponentAt(size_t index);
    void RemoveAllComponents();

    bool IsActive();
    void Activate(bool b);

    template<typename T>
    void RemoveComponentsByType() {
        auto it = std::remove_if(m_vComponents.begin(), m_vComponents.end(),
            [&](const std::shared_ptr<IComponent>& comp) {
                if (dynamic_cast<T*>(comp.get())) {
                    HandleComponentRemoval(comp.get());
                    return true;
                }
                return false;
            });
        m_vComponents.erase(it, m_vComponents.end());
    }

    template<typename Pred>
    void RemoveComponentsIf(Pred&& pred) {
        auto it = std::remove_if(m_vComponents.begin(), m_vComponents.end(),
            [&](const std::shared_ptr<IComponent>& comp) {
                if (pred(comp.get())) {
                    HandleComponentRemoval(comp.get());
                    return true;
                }
                return false;
            });
        m_vComponents.erase(it, m_vComponents.end());
    }

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
    
    template<typename Func>
    void ForEachComponent(Func&& func) {
        for (auto& comp : m_vComponents) {
            func(comp.get());
        }
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

    const std::string& GetName() const;
    void SetName(std::string name);

public:
    virtual void AfterCreation();
    virtual void Init();
    virtual void Destroy();

    void AddNotifyToPull(NRenderSystemNotifyType type);

public:
	
    REFLECT_BODY(ComponentHolder)
    BEGIN_REFLECT(ComponentHolder)
        REFLECT_PROPERTY(m_vComponents)
        REFLECT_PROPERTY(m_pWorld)
        REFLECT_PROPERTY(m_sName)
    END_REFLECT(ComponentHolder)

protected: 
    void HandleComponentRemoval(IComponent* comp);
    virtual void OnComponentRemoved(IComponent* comp);

protected:
    std::string m_sName = "Entity";
    World* m_pWorld = nullptr;
	std::vector<std::shared_ptr<IComponent>> m_vComponents;
};

inline bool operator==(const std::shared_ptr<ComponentHolder>& lhs, const std::shared_ptr<ComponentHolder>& rhs) {
    return lhs.get() == rhs.get();
}
inline bool operator==(const std::shared_ptr<ComponentHolder>& lhs, const ComponentHolder* rhs) {
    return lhs.get() == rhs;
}
inline bool operator==(const ComponentHolder* lhs, const std::shared_ptr<ComponentHolder>& rhs) {
    return lhs == rhs.get();
}
