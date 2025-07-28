#pragma once
#include <pch.h>

#include <MainRenderer/MainRendererComponent.h>
#include <Camera/MainRenderer_CameraComponent.h>
#include <WinWindow/Utils/Timer.h>
#include <D3DFramework/Objects/Scene.h>
#include <D3DFramework/Objects/SimpleObjects.h>
#include <RenderableObjects/RenderableMesh.h>
#include <RenderableObjects/RenderableSimplePlane.h>
#include <RenderableObjects/RenderableSimpleCone.h>
#include <RenderableObjects/RenderableSimpleCube.h>
#include <RenderableObjects/RenderableSimpleSphere.h>
#include <RenderableObjects/RenderableSkyboxObject.h>
#include <RenderableObjects/RenderableAudioObject.h>

template<typename TObject, typename = void>
struct RenderableType;

template<typename TObject>
struct RenderableType< TObject, std::enable_if_t<std::is_base_of_v<FD3DW::SimpleObject<FD3DW::SceneVertexFrameWork>, TObject>> > {
    using type = RenderableSimpleObject;
};

template<>
struct RenderableType<FD3DW::Scene> {
    using type = RenderableMesh;
};

template<>
struct RenderableType<FD3DW::Audio> {
    using type = RenderableAudioObject;
};

class MainRenderer_RenderableObjectsManager : public MainRendererComponent {
public:

    template<typename TRenderable, typename... Args>
    BaseRenderableObject* CreateObject(ID3D12Device* device, ID3D12GraphicsCommandList* list, Args&&... args) {
        
        auto renderable = std::make_unique<TRenderable>(std::forward<Args>(args)...);
        DoInitObject(renderable.get(), device, list);
        auto ptr = renderable.get();
        m_vObjects.push_back(std::move(renderable));
        return ptr;
    }

    void CreateSphere(ID3D12GraphicsCommandList* list);
    void CreateCube(ID3D12GraphicsCommandList* list);
    void CreateCone(ID3D12GraphicsCommandList* list);
    void CreatePlane(ID3D12GraphicsCommandList* list);

    void RemoveObject(BaseRenderableObject* obj);

    std::vector<BaseRenderableObject*> GetRenderableObjects() const;

    void BeforeRender(ID3D12GraphicsCommandList* cmdList);
    void DeferredRender(ID3D12GraphicsCommandList* list);
    void ForwardRender(ID3D12GraphicsCommandList* list);
    void AfterRender();


    virtual void AfterConstruction() override;
    virtual void BeforeDestruction() override;

    RenderableSkyboxObject* FindSkyboxObject();

    BEGIN_FIELD_REGISTRATION(MainRenderer_RenderableObjectsManager, MainRendererComponent)
        REGISTER_FIELD(m_vObjects)
    END_FIELD_REGISTRATION()

private:
    void DoInitObject(BaseRenderableObject* obj, ID3D12Device* device, ID3D12GraphicsCommandList* list);
    void SpecificPostLoadForOblect(BaseRenderableObject* obj);

private:
    void DoDeleteObject(BaseRenderableObject* obj);

private:

    std::vector < BaseRenderableObject* > m_vSheduleForDelete;

    std::vector<std::unique_ptr<BaseRenderableObject>> m_vObjects;
};