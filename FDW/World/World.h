#pragma once 

#include <pch.h>
#include <Entity/Core/ComponentHolder.h>
#include <Entity/RenderObject/TRender.h>
#include <Entity/Audio/TAudio.h>
#include <Entity/Camera/TDefaultCamera.h>
#include <Entity/Light/TDirectionalLight.h>
#include <Entity/Light/TPointLight.h>
#include <Entity/Light/TRectLight.h>
#include <Entity/Light/TSpotLight.h>
#include <Entity/RenderObject/SimpleMeshTemplates.h>
#include <Entity/RenderObject/TScene.h>
#include <Entity/RenderObject/TSkybox.h>

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
    void ClearNotifies();

public:
    ComponentHolder* CreateEmptyEntity();
    TDefaultCamera* CreateDefaultCamera();
    TDirectionalLight* CreateDirectionalLight();
    TPointLight* CreatePointLight();
    TSpotLight* CreateSpotLight();
    TRectLight* CreateRectLight();
    TSimplePlane* CreateSimplePlane();
    TSimpleCube* CreateSimpleCube();
    TSimpleCone* CreateSimpleCone();
    TSimpleSphere* CreateSimpleSphere();
    TSkybox* CreateSkybox(std::string path);
    TScene* CreateScene(std::string path);
    TAudio* CreateAudio(std::string path);

public:
    std::vector<ComponentHolder*> GetEntities();

    template<typename T>
    std::vector<T*> GetEntitiesByType() {
        std::vector<T*> result;
        for (auto& e : m_vEntities) {
            if (auto casted = dynamic_cast<T*>(e.get())) {
                result.push_back(casted);
            }
        }
        return result;
    }

public:
    template<typename T, typename... Args>
    T* CreateEntity(Args&&... args) {
        auto ent = std::make_shared<T>(std::forward<Args>(args)...);
        ent->SetWorld(this);
        ent->AfterCreation();
        ent->Init();
        auto entPtr = ent.get();
        if (auto* renderEntity = dynamic_cast<TRender*>(entPtr)){
            InitRenderEntity(renderEntity);
        }
        m_vEntities.push_back(ent);
        return ent.get();
    }

    void DestroyEntity(ComponentHolder* holder);

    template<typename T>
    std::vector<T*> GetAllComponentsOfType() {
        auto raw = CollectComponents([](IComponent* c) {
            return dynamic_cast<T*>(c) != nullptr;
            });
        std::vector<T*> result;
        result.reserve(raw.size());
        for (auto* c : raw) result.push_back(static_cast<T*>(c));
        return result;
    }

    template<typename... Types>
    std::vector<IComponent*> GetAllComponentsOfTypes() {
        return CollectComponents([](IComponent* c) {
            return ((dynamic_cast<Types*>(c) != nullptr) || ...);
            });
    }

    template<typename... Types>
    std::vector<IComponent*> GetAllComponentsExcept() {
        return CollectComponents([](IComponent* c) {
            return !((dynamic_cast<Types*>(c) != nullptr) || ...);
            });
    }
    template<typename... ExcludeTypes>
    struct Exclude {};

    template<typename... IncludeTypes, typename... ExcludeTypes>
    std::vector<IComponent*> GetComponentsIncludeExclude(Exclude<ExcludeTypes...>) {
        return CollectComponents([](IComponent* c) {
            bool include = ((dynamic_cast<IncludeTypes*>(c) != nullptr) || ...);
            bool exclude = ((dynamic_cast<ExcludeTypes*>(c) != nullptr) || ...);
            return include && !exclude;
            });
    }



private:
    void InitRenderEntity(TRender* render);

private:
    template<typename Filter>
    std::vector<IComponent*> CollectComponents(Filter&& filter) {
        std::vector<IComponent*> result;
        for (auto& e : m_vEntities) {
            for (auto& comp : e->GetComponents<IComponent>()) {
                if (filter(comp)) {
                    result.push_back(comp);
                }
            }
        }
        return result;
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
