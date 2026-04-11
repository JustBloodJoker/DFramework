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
#include <WinWindow/Utils/WorkerPool.h>

class MainRenderer;
class MeshComponent;

struct WorldScriptRecord {
    std::string Path;
    bool WasExecuted = false;

    REFLECT_STRUCT(WorldScriptRecord)
    BEGIN_REFLECT(WorldScriptRecord)
        REFLECT_PROPERTY(Path)
        REFLECT_PROPERTY(WasExecuted)
    END_REFLECT(WorldScriptRecord)
};

struct ScriptBoundsInfo {
    float MinX = 0.0f;
    float MinY = 0.0f;
    float MinZ = 0.0f;

    float MaxX = 0.0f;
    float MaxY = 0.0f;
    float MaxZ = 0.0f;

    float SizeX = 0.0f;
    float SizeY = 0.0f;
    float SizeZ = 0.0f;

    float CenterX = 0.0f;
    float CenterY = 0.0f;
    float CenterZ = 0.0f;

    REFLECT_STRUCT(ScriptBoundsInfo)
    BEGIN_REFLECT(ScriptBoundsInfo)
        REFLECT_PROPERTY(MinX)
        REFLECT_PROPERTY(MinY)
        REFLECT_PROPERTY(MinZ)
        REFLECT_PROPERTY(MaxX)
        REFLECT_PROPERTY(MaxY)
        REFLECT_PROPERTY(MaxZ)
        REFLECT_PROPERTY(SizeX)
        REFLECT_PROPERTY(SizeY)
        REFLECT_PROPERTY(SizeZ)
        REFLECT_PROPERTY(CenterX)
        REFLECT_PROPERTY(CenterY)
        REFLECT_PROPERTY(CenterZ)
    END_REFLECT(ScriptBoundsInfo)
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

    void ActivateCamera(ComponentHolder* entity);
    int FillEntityBounds(ComponentHolder* entity, ScriptBoundsInfo* outBounds);

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
    void ApplyScriptLightsState(void* lightsArray,void* xArray,void* yArray,void* zArray,void* intensityArray,void* kindArray,void* dirXArray,void* dirYArray,void* dirZArray,int beginIndex,int endIndex);
    void UpdateScriptLightsNative(void* lightsArray,void* kindArray,void* xArray,void* yArray,void* zArray,void* txArray,void* tyArray,void* tzArray,void* spdArray,void* distLimArray,void* yMinLimArray,void* yMaxLimArray,void* intenArray,void* idirArray,void* heatArray,void* iminArray,void* imaxArray,void* dirXArray,void* dirYArray,void* dirZArray,void* dirLerpArray,float dt,int beginIndex,int endIndex);

    void SetDefaultCameraEye(ComponentHolder* entity, float x, float y, float z);
    void SetDefaultCameraEyeAndLookAt(ComponentHolder* entity, float eyeX, float eyeY, float eyeZ, float lookX, float lookY, float lookZ);
    void SetDefaultCameraYawPitchRoll(ComponentHolder* entity, float yaw, float pitch, float roll);
    void SetDefaultCameraInputEnabled(ComponentHolder* entity, int enabled);
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
    void SetIBLEnabled(int enabled);
    void SetIBLDiffuseIntensity(float intensity);
    void SetIBLSpecularIntensity(float intensity);
    void SetIBLMaxReflectionMip(float value);
    void SetTAAEnabled(int enabled);
    void SetTAABlendWeight(float value);
    void SetJitterEnabled(int enabled);
    void SetLinkJitterToTAA(int enabled);
    void SetUnlitScene(int enabled);
    void SetMeshCullingType(int type);
    void SetScriptUpdateInterval(float seconds);
    void SetScriptUpdateMaxAccumulator(float seconds);
    void SetScriptNativeParallelUpdateEnabled(int enabled);
    void SetScriptNativeParallelWorkerCount(int workerCount);
    void SetScriptNativeParallelChunkSize(int chunkSize);
    void CinematicClearPath();
    void CinematicAddPoint(float eyeX, float eyeY, float eyeZ, float lookX, float lookY, float lookZ);
    void CinematicSetDuration(float durationSeconds);
    void CinematicSetLoop(int loop);
    void CinematicSetUseSmoothStep(int enabled);
    int CinematicPlay(ComponentHolder* cameraEntity, int lockInput);
    void CinematicStop();
    int CinematicIsPlaying();
    float CinematicGetProgress();
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
        REFLECT_METHOD(ActivateCamera)
        REFLECT_METHOD(FillEntityBounds)
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
        REFLECT_METHOD(ApplyScriptLightsState)
        REFLECT_METHOD(UpdateScriptLightsNative)
        REFLECT_METHOD(SetDefaultCameraEye)
        REFLECT_METHOD(SetDefaultCameraEyeAndLookAt)
        REFLECT_METHOD(SetDefaultCameraYawPitchRoll)
        REFLECT_METHOD(SetDefaultCameraInputEnabled)
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
        REFLECT_METHOD(SetIBLEnabled)
        REFLECT_METHOD(SetIBLDiffuseIntensity)
        REFLECT_METHOD(SetIBLSpecularIntensity)
        REFLECT_METHOD(SetIBLMaxReflectionMip)
        REFLECT_METHOD(SetTAAEnabled)
        REFLECT_METHOD(SetTAABlendWeight)
        REFLECT_METHOD(SetJitterEnabled)
        REFLECT_METHOD(SetLinkJitterToTAA)
        REFLECT_METHOD(SetUnlitScene)
        REFLECT_METHOD(SetMeshCullingType)
        REFLECT_METHOD(SetScriptUpdateInterval)
        REFLECT_METHOD(SetScriptUpdateMaxAccumulator)
        REFLECT_METHOD(SetScriptNativeParallelUpdateEnabled)
        REFLECT_METHOD(SetScriptNativeParallelWorkerCount)
        REFLECT_METHOD(SetScriptNativeParallelChunkSize)
        REFLECT_METHOD(CinematicClearPath)
        REFLECT_METHOD(CinematicAddPoint)
        REFLECT_METHOD(CinematicSetDuration)
        REFLECT_METHOD(CinematicSetLoop)
        REFLECT_METHOD(CinematicSetUseSmoothStep)
        REFLECT_METHOD(CinematicPlay)
        REFLECT_METHOD(CinematicStop)
        REFLECT_METHOD(CinematicIsPlaying)
        REFLECT_METHOD(CinematicGetProgress)
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
    bool BuildBoundsFromMeshes(const std::vector<MeshComponent*>& meshes, ScriptBoundsInfo& outBounds) const;
    void UpdateCinematic(float dt);
    void SetCinematicInputLock(bool lockInput);
    static float ApplyCinematicSmoothStep(float t);

    struct CinematicPoint {
        dx::XMFLOAT3 Eye = { 0.0f, 0.0f, 0.0f };
        dx::XMFLOAT3 LookAt = { 0.0f, 0.0f, 0.0f };
    };

    static dx::XMFLOAT3 CatmullRom(const dx::XMFLOAT3& p0, const dx::XMFLOAT3& p1, const dx::XMFLOAT3& p2, const dx::XMFLOAT3& p3, float t);
    static dx::XMFLOAT3 EvaluateCinematicSpline(const std::vector<CinematicPoint>& points, float t01, bool sampleLookAt);

protected:
    std::vector<std::shared_ptr<ComponentHolder>> m_vEntities;
    std::vector<WorldScriptRecord> m_vLoadedScripts;
    float m_fScriptUpdateAccumulator = 0.0f;
    float m_fScriptUpdateInterval = (1.0f / 15.0f);
    float m_fScriptUpdateMaxAccumulator = 0.2f;
    bool m_bScriptNativeParallelUpdateEnabled = true;
    int m_iScriptNativeParallelWorkerCount = 3;
    int m_iScriptNativeParallelChunkSize = 48;
    FDWWIN::WorkerPool m_xScriptNativeWorkerPool;
    bool m_bIsScriptNativeWorkerPoolInitialized = false;
    MainRenderer* m_pRender = nullptr;
    std::unordered_map<std::string, ComponentHolder*> m_mEntityByName;

protected:
    std::vector<CinematicPoint> m_vCinematicPoints;
    TDefaultCamera* m_pCinematicCamera = nullptr;
    float m_fCinematicDuration = 24.0f;
    float m_fCinematicTime = 0.0f;
    bool m_bCinematicPlaying = false;
    bool m_bCinematicLoop = false;
    bool m_bCinematicUseSmoothStep = true;
    bool m_bCinematicLockInput = true;
    int m_iCinematicPrevInputEnabled = 1;

protected:
    std::vector<NRenderSystemNotifyType> m_vRenderSystemNotifies;
};
