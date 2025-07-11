#pragma once
#include <pch.h>

#include <MainRenderer/MainRendererComponent.h>
#include <Camera/MainRenderer_CameraComponent.h>
#include <WinWindow/Utils/Timer.h>
#include <D3DFramework/Objects/Scene.h>
#include <D3DFramework/Objects/SimpleObjects.h>
#include <RenderableObjects/RenderableMesh.h>
#include <RenderableObjects/RenderableSimpleObject.h>

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

class MainRenderer_RenderableObjectsManager : public MainRendererComponent {
public:
    MainRenderer_RenderableObjectsManager(MainRenderer* owner);

    template<typename TObject>
    void CreateObject(std::unique_ptr<TObject> obj, ID3D12Device* device, ID3D12GraphicsCommandList* list) {
        using TRenderable = typename RenderableType<TObject>::type;

        static_assert(std::is_base_of_v<FD3DW::Object, TObject>, "TObject must derive from FD3DW::Object");
        static_assert(std::is_base_of_v<BaseRenderableObject, TRenderable>, "TRenderable must derive from BaseRenderableObject");

        auto renderable = std::make_unique<TRenderable>(std::move(obj));
        renderable->Init(device, list);
        m_vObjects.push_back(std::move(renderable));
    }

    void RemoveObject(BaseRenderableObject* obj);

    std::vector<BaseRenderableObject*> GetRenderableObjects() const;

    void RenderObjects(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);

private:
    std::vector<std::unique_ptr<BaseRenderableObject>> m_vObjects;
};