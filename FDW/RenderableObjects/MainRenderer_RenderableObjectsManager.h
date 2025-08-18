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
#include <RenderableObjects/ObjectsCulling/ObjectCulling.h>


enum class CullingType {
    None,
    CPUFrustum,
    GPUFrustum
};

class MainRenderer_RenderableObjectsManager : public MainRendererComponent {
public:

    template<typename TRenderable, typename... Args>
    TRenderable* CreateObject(ID3D12GraphicsCommandList* list, ID3D12GraphicsCommandList4* dxrList, Args&&... args) {
        
        auto renderable = std::make_unique<TRenderable>(std::forward<Args>(args)...);
        DoInitObject(renderable.get(), list, dxrList);
        auto ptr = renderable.get();
        m_vObjects.push_back(std::move(renderable));
        m_bIsNeedUpdateTLAS = true;
        return ptr;
    }

    RenderableSimpleSphere* CreateSphere(ID3D12GraphicsCommandList* list, ID3D12GraphicsCommandList4* dxrList);
    RenderableSimpleCube* CreateCube(ID3D12GraphicsCommandList* list, ID3D12GraphicsCommandList4* dxrList);
    RenderableSimpleCone* CreateCone(ID3D12GraphicsCommandList* list, ID3D12GraphicsCommandList4* dxrList);
    RenderableSimplePlane* CreatePlane(ID3D12GraphicsCommandList* list, ID3D12GraphicsCommandList4* dxrList);

    void RemoveObject(BaseRenderableObject* obj);

    std::vector<BaseRenderableObject*> GetRenderableObjects() const;

    void BeforeRender(ID3D12GraphicsCommandList* cmdList);
    void DeferredRender(ID3D12GraphicsCommandList* list);
    void ForwardRender(ID3D12GraphicsCommandList* list);
    void AfterRender();

    FD3DW::AccelerationStructureBuffers GetTLASData(ID3D12Device5* device, ID3D12GraphicsCommandList4* list);

    virtual void AfterConstruction() override;
    virtual void BeforeDestruction() override;

    bool IsNeedUpdateTLAS();

    RenderableSkyboxObject* FindSkyboxObject();

    void SetMeshCullingType(CullingType in);
    CullingType GetMeshCullingType();

public:

    BEGIN_FIELD_REGISTRATION(MainRenderer_RenderableObjectsManager, MainRendererComponent)
        REGISTER_FIELD(m_vObjects)
    END_FIELD_REGISTRATION()

private:
    void DoInitObject(BaseRenderableObject* obj, ID3D12GraphicsCommandList* list, ID3D12GraphicsCommandList4* dxrList);
    void SpecificPostLoadForOblect(BaseRenderableObject* obj);

private:
    void DoDefferedRender(ID3D12GraphicsCommandList* list, std::vector<BaseRenderableObject*> objectsToRender);
    void InitIndirectDeferredMeshExecution();

private:
    void DoDeleteObject(BaseRenderableObject* obj);

private:
    FD3DW::AccelerationStructureBuffers m_xTLASBufferData;
    bool m_bIsNeedUpdateTLAS = true;

    std::vector < BaseRenderableObject* > m_vSheduleForDelete;

    std::vector<std::unique_ptr<BaseRenderableObject>> m_vObjects;

private:
    //Indirect data
    wrl::ComPtr<ID3D12CommandSignature> m_pIndirectDeferredFirstPassCommandSignature = nullptr;
    std::unique_ptr<FD3DW::StructuredBuffer> m_pIndirectDeferredFirstPassCommandsBuffer = nullptr;

    CullingType m_xCullingType = CullingType::None;
    std::unique_ptr<ObjectCulling> m_pObjectCulling = nullptr;
};