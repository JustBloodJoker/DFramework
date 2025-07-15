#include <RenderableObjects/MainRenderer_RenderableObjectsManager.h>
#include <MainRenderer/MainRenderer.h>

MainRenderer_RenderableObjectsManager::MainRenderer_RenderableObjectsManager(MainRenderer* owner) 
    : MainRendererComponent(owner)
{
}

void MainRenderer_RenderableObjectsManager::CreateObject(const std::string path, ID3D12Device* device, ID3D12GraphicsCommandList* list) {
    if (auto skybox = FindSkyboxObject())
    {
        RemoveObject(skybox);
    }

    auto renderable = std::make_unique<RenderableSkyboxObject>(path);
    renderable->Init(device, list);
    m_vObjects.push_back(std::move(renderable));
    
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

void MainRenderer_RenderableObjectsManager::BeforeRender(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList) {
    auto timer = m_pOwner->GetTimer();

    BeforeRenderInputData data;
    data.Time = timer->GetTime();
    data.DT = timer->GetDeltaTime();
    data.Device = device;
    data.CommandList = cmdList;
    data.Projection = m_pOwner->GetCurrentProjectionMatrix();
    data.View = m_pOwner->GetCurrentViewMatrix();
    data.AdditionalWorld = dx::XMMatrixIdentity();

    for (auto& obj : m_vObjects) {
        obj->BeforeRender(data);
    }
}

void MainRenderer_RenderableObjectsManager::DeferredRender(ID3D12GraphicsCommandList* list) {
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

RenderableSkyboxObject* MainRenderer_RenderableObjectsManager::FindSkyboxObject() {
    for (const auto& obj : m_vObjects) {
        if (auto skyBox = dynamic_cast<RenderableSkyboxObject*>(obj.get())) return skyBox;
    }
    return nullptr;
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
