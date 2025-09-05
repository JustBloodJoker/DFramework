#pragma once

#include <pch.h>
#include <D3DFramework/Utilites/Serializer/ReflectionImpl.h>
#include <Entity/Core/IComponent.h>
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
        m_vComponents.push_back(comp);
        return comp.get();
    }

    void RemoveComponent(IComponent* ptr);
    void RemoveComponentAt(size_t index);
    void RemoveAllComponents();

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

public:
    void SetWorld(World* world);
    World* GetWorld();

    const std::string& GetName() const;

public:
    virtual void AfterCreation();
    virtual void Init();
    virtual void BeforeRenderTick(float dt);
    virtual void AfterRenderTick(float dt);
    virtual void Destroy();

    void AddNotifyToPull(NRenderSystemNotifyType type);

public:
	
	BEGIN_FIELD_REGISTRATION(ComponentHolder)
		REGISTER_FIELD(m_vComponents)
		REGISTER_FIELD(m_pWorld)
		REGISTER_FIELD(m_sName)
	END_FIELD_REGISTRATION();

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
