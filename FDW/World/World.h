#pragma once 

#include <pch.h>
#include <Entity/Core/ComponentHolder.h>

class MainRenderer;

class World {
public:
    World() = default;
    virtual ~World() = default;


public:
    void SetMainRenderer(MainRenderer* mr);
    MainRenderer* GetMainRenderer();

public:
    void AddNotifyToPull(NRenderSystemNotifyType type);
    std::vector<NRenderSystemNotifyType> GetRenderSystemNotifies() const;


public:
    template<typename T, typename... Args>
    T* CreateEntity(Args&&... args) {
        auto ent = std::make_shared<T>(std::forward<Args>(args)...);
        ent->SetWorld(this);
        ent->AfterCreation();
        ent->Init();
        m_vEntities.push_back(ent);
        return ent.get();
    }

    void DestroyEntity(ComponentHolder* holder);

    template<typename T>
    std::vector<T*> GetAllComponentsOfType() {
        std::vector<T*> result;
        for (auto& e : m_vEntities) {
            if (auto comp = e->GetComponent<T>()) {
                result.push_back(comp);
            }
        }
        return result;
    }

    template<typename... Types>
    std::vector<IComponent*> GetAllComponentsOfTypes() {
        std::vector<IComponent*> result;
        for (auto& e : m_vEntities) {
            (CollectComponent<Types>(e.get(), result), ...);
        }
        return result;
    }

    template<typename... Types>
    std::vector<IComponent*> GetAllComponentsExcept() {
        std::vector<IComponent*> result;
        for (auto& e : m_vEntities) {
            for (auto& comp : e->m_vComponents) {
                if (!((dynamic_cast<Types*>(comp.get())) || ...)) {
                    result.push_back(comp.get());
                }
            }
        }
        return result;
    }

    template<typename... IncludeTypes, typename... ExcludeTypes>
    std::vector<IComponent*> GetComponentsIncludeExcludeHelper(std::tuple<ExcludeTypes...>) {
        std::vector<IComponent*> result;
        for (auto& e : m_vEntities) {
            (CollectComponent<IncludeTypes>(e.get(), result), ...);

            result.erase(
                std::remove_if(result.begin(), result.end(),
                    [](IComponent* comp) {
                        return ((dynamic_cast<ExcludeTypes*>(comp)) || ...);
                    }),
                result.end()
            );
        }
        return result;
    }


private:
    template<typename T>
    void CollectComponent(ComponentHolder* entity, std::vector<IComponent*>& out) {
        if (auto comp = entity->GetComponent<T>()) {
            out.push_back(comp);
        }
    }

public:
    BEGIN_FIELD_REGISTRATION(World)
        REGISTER_FIELD(m_vEntities)
    END_FIELD_REGISTRATION();


protected:
    void AfterSetMainRenderer();

protected:
    std::vector<std::shared_ptr<ComponentHolder>> m_vEntities;
    MainRenderer* m_pRender = nullptr;

    std::vector<NRenderSystemNotifyType> m_vRenderSystemNotifies;
};
