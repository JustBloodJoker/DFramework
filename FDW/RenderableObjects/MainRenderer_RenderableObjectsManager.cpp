#include <RenderableObjects/MainRenderer_RenderableObjectsManager.h>
#include <MainRenderer/MainRenderer.h>
#include <MainRenderer/PSOManager.h>
#include <RenderableObjects/GeneratorsForSimpleObjects.h>


RenderableSimpleSphere* MainRenderer_RenderableObjectsManager::CreateSphere(ID3D12GraphicsCommandList* list) {
    auto device = m_pOwner->GetDevice();
    return CreateObject<RenderableSimpleSphere>(device, list);
}

RenderableSimpleCube* MainRenderer_RenderableObjectsManager::CreateCube(ID3D12GraphicsCommandList* list) {
    auto device = m_pOwner->GetDevice();
    return CreateObject<RenderableSimpleCube>(device, list);
}

RenderableSimpleCone* MainRenderer_RenderableObjectsManager::CreateCone(ID3D12GraphicsCommandList* list) {
    auto device = m_pOwner->GetDevice();
    return CreateObject<RenderableSimpleCone>(device, list);
}

RenderableSimplePlane* MainRenderer_RenderableObjectsManager::CreatePlane(ID3D12GraphicsCommandList* list) {
    auto device = m_pOwner->GetDevice();
    return CreateObject<RenderableSimplePlane>(device, list);
}

void MainRenderer_RenderableObjectsManager::RemoveObject(BaseRenderableObject* obj) {
    m_vSheduleForDelete.push_back(obj);
}

std::vector<BaseRenderableObject*> MainRenderer_RenderableObjectsManager::GetRenderableObjects() const {
    std::vector<BaseRenderableObject*> ret;
    ret.reserve(m_vObjects.size());

    for (const auto& obj : m_vObjects) {
        ret.push_back(obj.get());
    }

    return ret;
}

void MainRenderer_RenderableObjectsManager::BeforeRender(ID3D12GraphicsCommandList* cmdList) {
    auto timer = m_pOwner->GetTimer();

    BeforeRenderInputData data;
    data.Time = timer->GetTime();
    data.DT = timer->GetDeltaTime();
    data.Device = m_pOwner->GetDevice();
    data.CommandList = cmdList;
    data.Projection = m_pOwner->GetCurrentProjectionMatrix();
    data.View = m_pOwner->GetCurrentViewMatrix();
    data.CameraPosition = m_pOwner->GetCurrentCameraPosition();
    data.AdditionalWorld = dx::XMMatrixIdentity();

    for (auto& obj : m_vObjects) {
        obj->BeforeRender(data);
    }
}

void MainRenderer_RenderableObjectsManager::DeferredRender(ID3D12GraphicsCommandList* list) {
    PSOManager::GetInstance()->GetPSOObject(PSOType::DefferedFirstPassDefaultConfig)->Bind(list);
    for (auto& obj : m_vObjects) {
        if(obj->IsCanRenderInPass(RenderPass::Deferred)) obj->DeferredRender(list);
    }
}

void MainRenderer_RenderableObjectsManager::ForwardRender(ID3D12GraphicsCommandList* list) {
    for (auto& obj : m_vObjects) {
        if (obj->IsCanRenderInPass(RenderPass::Forward)) obj->ForwardRender(list);
    }
}

void MainRenderer_RenderableObjectsManager::AfterRender() {
    for (const auto& obj : m_vSheduleForDelete) {
        DoDeleteObject(obj);
    }
    m_vSheduleForDelete.clear();

    FD3DW::FResource::ReleaseUploadBuffers();
}

void MainRenderer_RenderableObjectsManager::AfterConstruction() {
    auto cmList = m_pOwner->GetBindedCommandList();
    auto device = m_pOwner->GetDevice();
    for (const auto& obj : m_vObjects) {
        DoInitObject(obj.get(), device, cmList);
    }
}

void MainRenderer_RenderableObjectsManager::BeforeDestruction() {
    AfterRender();
}

RenderableSkyboxObject* MainRenderer_RenderableObjectsManager::FindSkyboxObject() {
    for (const auto& obj : m_vObjects) {
        if (auto skyBox = dynamic_cast<RenderableSkyboxObject*>(obj.get())) return skyBox;
    }
    return nullptr;
}

void MainRenderer_RenderableObjectsManager::DoInitObject(BaseRenderableObject* obj, ID3D12Device* device, ID3D12GraphicsCommandList* list) {
    obj->Init(device, list);
    SpecificPostLoadForOblect(obj);
}

void MainRenderer_RenderableObjectsManager::SpecificPostLoadForOblect(BaseRenderableObject* obj) {
    if (auto audio = dynamic_cast<RenderableAudioObject*>(obj)) {
        audio->CreateAfterLoadAudio(m_pOwner->GetAudioMananger());
    }
}

void MainRenderer_RenderableObjectsManager::DoDeleteObject(BaseRenderableObject* obj) {
    auto it = std::remove_if(m_vObjects.begin(), m_vObjects.end(),
        [obj](const std::unique_ptr<BaseRenderableObject>& o) {
            return o.get() == obj;
        });

    if (it != m_vObjects.end()) {
        m_vObjects.erase(it, m_vObjects.end());
    }
}
