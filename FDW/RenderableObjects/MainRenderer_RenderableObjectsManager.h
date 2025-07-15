#pragma once
#include <pch.h>

#include <MainRenderer/MainRendererComponent.h>
#include <Camera/MainRenderer_CameraComponent.h>
#include <WinWindow/Utils/Timer.h>
#include <D3DFramework/Objects/Scene.h>
#include <D3DFramework/Objects/SimpleObjects.h>
#include <RenderableObjects/RenderableMesh.h>
#include <RenderableObjects/RenderableSimpleObject.h>
#include <RenderableObjects/RenderableSkyboxObject.h>
#include <RenderableObjects/RenderableAudioObject.h>

template<typename TObject>
struct RenderableType;

template<>
struct RenderableType<FD3DW::Scene> {
    using type = RenderableMesh;
};

template<>
struct RenderableType<FD3DW::SimpleObject> {
    using type = RenderableSimpleObject;
};

template<>
struct RenderableType<FD3DW::Audio> {
    using type = RenderableAudioObject;
};

class MainRenderer_RenderableObjectsManager : public MainRendererComponent {
public:
    MainRenderer_RenderableObjectsManager(MainRenderer* owner);

    template<typename TObject, typename... Args>
    void CreateObject(std::unique_ptr<TObject> obj, ID3D12Device* device, ID3D12GraphicsCommandList* list, Args&&... args) {
        using TRenderable = typename RenderableType<TObject>::type;

        static_assert(std::is_base_of_v<BaseRenderableObject, TRenderable>, "TRenderable must derive from BaseRenderableObject");

        auto renderable = std::make_unique<TRenderable>(std::move(obj), std::forward<Args>(args)...);
        renderable->Init(device, list);
        m_vObjects.push_back(std::move(renderable));
    }

    void CreateObject(const std::string path, ID3D12Device* device, ID3D12GraphicsCommandList* list);

    void RemoveObject(BaseRenderableObject* obj);

    std::vector<BaseRenderableObject*> GetRenderableObjects() const;

    void BeforeRender(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);
    void DeferredRender(ID3D12GraphicsCommandList* list);
    void ForwardRender(ID3D12GraphicsCommandList* list);
    void AfterRender();

    RenderableSkyboxObject* FindSkyboxObject();

private:
    void DoDeleteObject(BaseRenderableObject* obj);

private:

    std::vector < BaseRenderableObject* > m_vSheduleForDelete;

    std::vector<std::unique_ptr<BaseRenderableObject>> m_vObjects;
};