#include <RenderableObjects/MainRenderer_RenderableObjectsManager.h>
#include <MainRenderer/MainRenderer.h>

MainRenderer_RenderableObjectsManager::MainRenderer_RenderableObjectsManager(MainRenderer* owner) 
    : MainRendererComponent(owner)
{
}

void MainRenderer_RenderableObjectsManager::RemoveObject(BaseRenderableObject* obj) {
    auto it = std::remove_if(m_vObjects.begin(), m_vObjects.end(),
        [obj](const std::unique_ptr<BaseRenderableObject>& o) {
            return o.get() == obj;
        });

    if (it != m_vObjects.end()) {
        m_vObjects.erase(it, m_vObjects.end());
    }
}

std::vector<BaseRenderableObject*> MainRenderer_RenderableObjectsManager::GetRenderableObjects() const {
    std::vector<BaseRenderableObject*> ret;
    ret.reserve(m_vObjects.size());

    for (const auto& obj : m_vObjects) {
        ret.push_back(obj.get());
    }

    return ret;
}

void MainRenderer_RenderableObjectsManager::RenderObjects(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList) {
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

    for (auto& obj : m_vObjects) {
        obj->Render(cmdList);
    }

}
