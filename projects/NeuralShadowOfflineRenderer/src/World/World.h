#pragma once

#include <Entity/Core/ComponentHolder.h>
#include <Entity/RenderObject/TRender.h>
#include <Entity/Camera/TDefaultCamera.h>
#include <Entity/Light/TDirectionalLight.h>
#include <Entity/Light/TPointLight.h>
#include <Entity/Light/TRectLight.h>
#include <Entity/Light/TSpotLight.h>
#include <Entity/RenderObject/SimpleMeshTemplates.h>
#include <Entity/RenderObject/TScene.h>

class OfflineRenderer;

class World {
public:
    World() = default;
    virtual ~World() = default;

public:
    void SetOfflineRenderer(OfflineRenderer* renderer);

public:
    void AddNotifyToPull(NRenderSystemNotifyType type);
    std::vector<NRenderSystemNotifyType> GetRenderSystemNotifies() const;
    void ClearNotifies();

public:
    TDefaultCamera* CreateDefaultCamera();
    TDirectionalLight* CreateDirectionalLight();
    TPointLight* CreatePointLight();
    TSpotLight* CreateSpotLight();
    TRectLight* CreateRectLight();
    TSimplePlane* CreateSimplePlane();
    TSimpleCube* CreateSimpleCube();
    TSimpleCone* CreateSimpleCone();
    TSimpleSphere* CreateSimpleSphere();
    TSimpleCylinder* CreateSimpleCylinder();
    TScene* CreateScene(std::string path);

public:
    template<typename T>
    std::vector<T*> GetEntitiesByType() {
        std::vector<T*> result;
        for (auto& entity : m_vEntities) {
            if (auto* casted = dynamic_cast<T*>(entity.get())) {
                result.push_back(casted);
            }
        }
        return result;
    }

    template<typename T, typename... Args>
    T* CreateEntity(Args&&... args) {
        auto entity = std::make_shared<T>(std::forward<Args>(args)...);
        entity->SetWorld(this);
        entity->AfterCreation();
        entity->Init();
        if (auto renderEntity = std::dynamic_pointer_cast<TRender>(entity)) {
            InitRenderEntity(renderEntity);
        }
        m_vEntities.push_back(entity);
        return entity.get();
    }

    template<typename T>
    std::vector<T*> GetAllComponentsOfType() {
        std::vector<T*> result;
        for (auto& entity : m_vEntities) {
            for (auto* component : entity->GetComponents<T>()) {
                result.push_back(component);
            }
        }
        return result;
    }

public:
    REFLECT_BODY(World)
    BEGIN_REFLECT(World)
        REFLECT_PROPERTY(m_vEntities)
    END_REFLECT(World)

protected:
    void InitRenderEntity(const std::shared_ptr<TRender>& render);
    void InitializeLoadedEntities();

protected:
    std::vector<std::shared_ptr<ComponentHolder>> m_vEntities;
    OfflineRenderer* m_pRender = nullptr;
    std::vector<NRenderSystemNotifyType> m_vRenderSystemNotifies;
};
