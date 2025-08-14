#include <RenderableObjects/MainRenderer_RenderableObjectsManager.h>
#include <MainRenderer/MainRenderer.h>
#include <MainRenderer/PSOManager.h>
#include <RenderableObjects/GeneratorsForSimpleObjects.h>


RenderableSimpleSphere* MainRenderer_RenderableObjectsManager::CreateSphere(ID3D12GraphicsCommandList* list, ID3D12GraphicsCommandList4* dxrList) {
    return CreateObject<RenderableSimpleSphere>(list, dxrList);
}

RenderableSimpleCube* MainRenderer_RenderableObjectsManager::CreateCube(ID3D12GraphicsCommandList* list, ID3D12GraphicsCommandList4* dxrList) {
    return CreateObject<RenderableSimpleCube>(list, dxrList);
}

RenderableSimpleCone* MainRenderer_RenderableObjectsManager::CreateCone(ID3D12GraphicsCommandList* list, ID3D12GraphicsCommandList4* dxrList) {
    return CreateObject<RenderableSimpleCone>(list, dxrList);
}

RenderableSimplePlane* MainRenderer_RenderableObjectsManager::CreatePlane(ID3D12GraphicsCommandList* list, ID3D12GraphicsCommandList4* dxrList) {
    return CreateObject<RenderableSimplePlane>(list, dxrList);
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

    RenderableObjectsMatricesBuffer::GetInstance()->BeforeRender(cmdList, m_pOwner->GetDevice());
    RenderableObjectsMaterialsBuffer::GetInstance()->BeforeRender(cmdList, m_pOwner->GetDevice());
    RenderableObjectsBoneMatricesBuffer::GetInstance()->BeforeRender(cmdList, m_pOwner->GetDevice());
}

void MainRenderer_RenderableObjectsManager::DeferredRender(ID3D12GraphicsCommandList* list) {
    PSOManager::GetInstance()->GetPSOObject(PSOType::DefferedFirstPassDefaultConfig)->Bind(list);

    RenderableObjectsMatricesBuffer::GetInstance()->SetBufferState(list, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    RenderableObjectsMaterialsBuffer::GetInstance()->SetBufferState(list, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    RenderableObjectsBoneMatricesBuffer::GetInstance()->SetBufferState(list, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    list->SetGraphicsRootShaderResourceView(CONSTANT_BUFFER_MATRICES_POSITION_IN_ROOT_SIG, RenderableObjectsMatricesBuffer::GetInstance()->GetGPUBufferAddress());
    list->SetGraphicsRootShaderResourceView(CONSTANT_BUFFER_MATERIALS_POSITION_IN_ROOT_SIG, RenderableObjectsMaterialsBuffer::GetInstance()->GetGPUBufferAddress());
    list->SetGraphicsRootShaderResourceView(ANIMATIONS_CONSTANT_BUFFER_IN_ROOT_SIG, RenderableObjectsBoneMatricesBuffer::GetInstance()->GetGPUBufferAddress());

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

FD3DW::AccelerationStructureBuffers MainRenderer_RenderableObjectsManager::GetTLASData(ID3D12Device5* device, ID3D12GraphicsCommandList4* list) {
    if (m_pOwner->IsRTSupported() && IsNeedUpdateTLAS() && !m_vObjects.empty()) {
        std::vector<std::pair<FD3DW::AccelerationStructureBuffers, dx::XMMATRIX>> instances;
        for (const auto& object : m_vObjects) {
            if (!object->IsCanRenderInPass(RenderPass::Deferred)) continue;

            auto objInstances = object->GetBLASInstances();
            instances.insert(instances.end(), objInstances.begin(), objInstances.end());
        }
        if (instances.empty()) {
            m_xTLASBufferData = {};
        }
        else {
            FD3DW::UpdateTopLevelAS(device, list, m_xTLASBufferData, instances);
        }
    }
    else if (m_vObjects.empty()) {
        m_xTLASBufferData = {};
    }

    m_bIsNeedUpdateTLAS = false;
    for (const auto& obj : m_vObjects) {
        obj->AfterTLASUpdate();
    }

    return m_xTLASBufferData;
}


bool MainRenderer_RenderableObjectsManager::IsNeedUpdateTLAS() {
    if (m_bIsNeedUpdateTLAS) return true;

    for (const auto& obj : m_vObjects) {
        if(obj->IsNeedUpdateTLAS()) return true;
    }

    return false;
}

void MainRenderer_RenderableObjectsManager::AfterConstruction() {
    RenderableObjectsMaterialsBuffer::GetInstance()->Init(RENDERABLE_OBJECTS_MATERIALS_BUFFER_PRECACHE_SIZE, m_pOwner->GetDevice());
    RenderableObjectsMatricesBuffer::GetInstance()->Init(RENDERABLE_OBJECTS_MATRICES_BUFFER_PRECACHE_SIZE, m_pOwner->GetDevice());
    RenderableObjectsBoneMatricesBuffer::GetInstance()->Init(RENDERABLE_OBJECTS_BONE_MATRICES_BUFFER_PRECACHE_SIZE, m_pOwner->GetDevice());

    auto cmList = m_pOwner->GetBindedCommandList();
    auto dxrList = m_pOwner->GetDXRCommandList();
    for (const auto& obj : m_vObjects) {
        DoInitObject(obj.get(), cmList, dxrList);
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

void MainRenderer_RenderableObjectsManager::DoInitObject(BaseRenderableObject* obj, ID3D12GraphicsCommandList* list, ID3D12GraphicsCommandList4* dxrList) {
    auto device = m_pOwner->GetDevice();
    obj->Init(device, list);

    SpecificPostLoadForOblect(obj);

    if (m_pOwner->IsRTSupported() && dxrList != nullptr) {
        auto dxrDevice = m_pOwner->GetDXRDevice();
        obj->InitBLASBuffers(dxrDevice, dxrList);
    }
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
        (*it)->BeforeDelete();
        m_vObjects.erase(it, m_vObjects.end());
    }

    m_bIsNeedUpdateTLAS = true;
}
