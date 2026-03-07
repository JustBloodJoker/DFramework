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

struct WorldScriptRecord {
    std::string Path;
    bool WasExecuted = false;

    REFLECT_STRUCT(WorldScriptRecord)
    BEGIN_REFLECT(WorldScriptRecord)
        REFLECT_PROPERTY(Path)
        REFLECT_PROPERTY(WasExecuted)
    END_REFLECT(WorldScriptRecord)
};

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
    ComponentHolder* GetEntityByName(std::string name);
    void SetEntityName(ComponentHolder* entity, std::string name);
    void SetEntityActive(ComponentHolder* entity, int active);

    void SetMeshPosition(ComponentHolder* entity, float x, float y, float z);
    void SetMeshRotation(ComponentHolder* entity, float x, float y, float z);
    void SetMeshScale(ComponentHolder* entity, float x, float y, float z);
    void TranslateMesh(ComponentHolder* entity, float dx, float dy, float dz);

    void SetLightColor(ComponentHolder* entity, float r, float g, float b);
    void SetLightIntensity(ComponentHolder* entity, float intensity);
    void SetLightPosition(ComponentHolder* entity, float x, float y, float z);
    void SetLightDirection(ComponentHolder* entity, float x, float y, float z);
    void SetLightAttenuationRadius(ComponentHolder* entity, float radius);
    void SetLightSourceRadius(ComponentHolder* entity, float radius);
    void SetLightConeAngles(ComponentHolder* entity, float inner, float outer);
    void SetRectLightSize(ComponentHolder* entity, float x, float y);
    void SetRectLightRotation(ComponentHolder* entity, float x, float y, float z);

    void SetDefaultCameraEye(ComponentHolder* entity, float x, float y, float z);
    void SetDefaultCameraYawPitchRoll(ComponentHolder* entity, float yaw, float pitch, float roll);
    void SetCameraSpeed(ComponentHolder* entity, float speed);
    void ResetCamera(ComponentHolder* entity);
    void MoveDefaultCameraForward(ComponentHolder* entity, float dt);
    void MoveDefaultCameraBackward(ComponentHolder* entity, float dt);
    void MoveDefaultCameraLeft(ComponentHolder* entity, float dt);
    void MoveDefaultCameraRight(ComponentHolder* entity, float dt);

    void PlayAudio(ComponentHolder* entity);
    void StopAudio(ComponentHolder* entity);
    void RestartAudio(ComponentHolder* entity);
    void SetAudioVolume(ComponentHolder* entity, float volume);
    void SetAudioLoop(ComponentHolder* entity, int loop);

public:
    void SetPreDepthEnabled(int enabled);
    void SetBloomEnabled(int enabled);
    void SetBloomThreshold(float threshold);
    void SetBloomIntensity(float intensity);
    void SetBloomBlurType(int blurType);
    void LoadWorldFile(std::string path);
    void SaveWorldFile(std::string path);

public:
    int ExecuteScriptFile(std::string path);
    void UpdateScripts(float dt);
    const std::vector<WorldScriptRecord>& GetLoadedScripts() const;

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
        if (auto renderEntity = std::dynamic_pointer_cast<TRender>(ent)) {
            InitRenderEntity(renderEntity);
        }
        m_vEntities.push_back(ent);
        AddEntityNameToCache(ent.get());
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
    void InitRenderEntity(std::shared_ptr<TRender> render);

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
    REFLECT_BODY(World)
    BEGIN_REFLECT(World)
        REFLECT_PROPERTY(m_vEntities)
        REFLECT_PROPERTY(m_vLoadedScripts)
        REFLECT_METHOD(CreateEmptyEntity)
        REFLECT_METHOD(CreateDefaultCamera)
        REFLECT_METHOD(CreateDirectionalLight)
        REFLECT_METHOD(CreatePointLight)
        REFLECT_METHOD(CreateSpotLight)
        REFLECT_METHOD(CreateRectLight)
        REFLECT_METHOD(CreateSimplePlane)
        REFLECT_METHOD(CreateSimpleCube)
        REFLECT_METHOD(CreateSimpleCone)
        REFLECT_METHOD(CreateSimpleSphere)
        REFLECT_METHOD(CreateSkybox)
        REFLECT_METHOD(CreateScene)
        REFLECT_METHOD(CreateAudio)
        REFLECT_METHOD(GetEntityByName)
        REFLECT_METHOD(SetEntityName)
        REFLECT_METHOD(SetEntityActive)
        REFLECT_METHOD(DestroyEntity)
        REFLECT_METHOD(SetMeshPosition)
        REFLECT_METHOD(SetMeshRotation)
        REFLECT_METHOD(SetMeshScale)
        REFLECT_METHOD(TranslateMesh)
        REFLECT_METHOD(SetLightColor)
        REFLECT_METHOD(SetLightIntensity)
        REFLECT_METHOD(SetLightPosition)
        REFLECT_METHOD(SetLightDirection)
        REFLECT_METHOD(SetLightAttenuationRadius)
        REFLECT_METHOD(SetLightSourceRadius)
        REFLECT_METHOD(SetLightConeAngles)
        REFLECT_METHOD(SetRectLightSize)
        REFLECT_METHOD(SetRectLightRotation)
        REFLECT_METHOD(SetDefaultCameraEye)
        REFLECT_METHOD(SetDefaultCameraYawPitchRoll)
        REFLECT_METHOD(SetCameraSpeed)
        REFLECT_METHOD(ResetCamera)
        REFLECT_METHOD(MoveDefaultCameraForward)
        REFLECT_METHOD(MoveDefaultCameraBackward)
        REFLECT_METHOD(MoveDefaultCameraLeft)
        REFLECT_METHOD(MoveDefaultCameraRight)
        REFLECT_METHOD(PlayAudio)
        REFLECT_METHOD(StopAudio)
        REFLECT_METHOD(RestartAudio)
        REFLECT_METHOD(SetAudioVolume)
        REFLECT_METHOD(SetAudioLoop)
        REFLECT_METHOD(SetPreDepthEnabled)
        REFLECT_METHOD(SetBloomEnabled)
        REFLECT_METHOD(SetBloomThreshold)
        REFLECT_METHOD(SetBloomIntensity)
        REFLECT_METHOD(SetBloomBlurType)
        REFLECT_METHOD(LoadWorldFile)
        REFLECT_METHOD(SaveWorldFile)
        REFLECT_METHOD(ExecuteScriptFile)
    END_REFLECT(World)


protected:
    void AfterSetMainRenderer();
    void SetupScriptingBindings();
    void RestoreScriptPredicatesAfterLoad();
    void RememberExecutedScript(const std::string& path);
    void AddEntityNameToCache(ComponentHolder* entity);
    void RemoveEntityNameFromCache(ComponentHolder* entity);
    void RebuildEntityNameCache();

protected:
    std::vector<std::shared_ptr<ComponentHolder>> m_vEntities;
    std::vector<WorldScriptRecord> m_vLoadedScripts;
    MainRenderer* m_pRender = nullptr;
    std::unordered_map<std::string, ComponentHolder*> m_mEntityByName;

    std::vector<NRenderSystemNotifyType> m_vRenderSystemNotifies;
};
