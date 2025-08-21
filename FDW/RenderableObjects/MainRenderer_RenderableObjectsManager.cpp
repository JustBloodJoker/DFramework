#include <RenderableObjects/MainRenderer_RenderableObjectsManager.h>
#include <MainRenderer/MainRenderer.h>
#include <MainRenderer/PSOManager.h>
#include <MainRenderer/GlobalTextureHeap.h>
#include <RenderableObjects/GeneratorsForSimpleObjects.h>
#include <RenderableObjects/IndirectExecutionMeshObject.h>

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

    for (auto& obj : m_vObjects) {
        obj->BeforeRender(data);
    }
}

void MainRenderer_RenderableObjectsManager::PreDepthRender(ID3D12GraphicsCommandList* list)
{
    for (auto& obj : m_vObjects) {
        obj->PreDepthRender(list);
    }
}

void MainRenderer_RenderableObjectsManager::DeferredRender(ID3D12GraphicsCommandList* list) {
    std::vector<BaseRenderableObject*> objectsToRender;
    for (auto& obj : m_vObjects) {
        if (obj->IsCanRenderInPass(RenderPass::Deferred)) objectsToRender.push_back(obj.get());
    }
    DoDefferedRender(list, objectsToRender);
}

void MainRenderer_RenderableObjectsManager::ForwardRender(ID3D12GraphicsCommandList* list) {
    for (auto& obj : m_vObjects) {
        if (obj->IsCanRenderInPass(RenderPass::Forward)) obj->ForwardRender(list);
    }
}

void MainRenderer_RenderableObjectsManager::AfterRender() {
    if (m_vSheduleForDelete.empty()) return;

    GlobalRenderThreadManager::GetInstance()->WaitIdle();

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
    InitIndirectDeferredMeshExecution();

    if (m_vObjects.empty()) return;

    GlobalRenderThreadManager::GetInstance()->WaitIdle();
    std::shared_ptr<FD3DW::ICommandRecipe> recipe = nullptr;
    if (m_pOwner->IsRTSupported()) {
        recipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList4>>(D3D12_COMMAND_LIST_TYPE_DIRECT, [this](ID3D12GraphicsCommandList4* list) {
            for (const auto& obj : m_vObjects) {
                DoInitObject(obj.get(), list, list);
            }
        });
    }
    else {
        recipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_DIRECT, [this](ID3D12GraphicsCommandList* list) {
            for (const auto& obj : m_vObjects) {
                DoInitObject(obj.get(), list, nullptr);
            }
        });
    }
    GlobalRenderThreadManager::GetInstance()->Submit(recipe);
    GlobalRenderThreadManager::GetInstance()->WaitIdle();
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

void MainRenderer_RenderableObjectsManager::SetMeshCullingType(CullingType in) { m_xCullingType = in; }

CullingType MainRenderer_RenderableObjectsManager::GetMeshCullingType() { return m_xCullingType; }

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

std::vector<IndirectMeshRenderableData> datas;
void MainRenderer_RenderableObjectsManager::DoDefferedRender(ID3D12GraphicsCommandList* list, std::vector<BaseRenderableObject*> objectsToRender) {
    std::vector<IndirectExecutionMeshObject*> objectsForIndirectExecute;
    std::vector<BaseRenderableObject*> objectsForDefaultRender;
    
    for (const auto& obj : objectsToRender) {
        auto indirect = dynamic_cast<IndirectExecutionMeshObject*>(obj);
        if (indirect && indirect->IsCanBeIndirectExecuted()) {
            objectsForIndirectExecute.push_back(indirect);
        }
        else {
            objectsForDefaultRender.push_back(obj);
        }
    }

    const auto& frustum = m_pOwner->GetCameraFrustum();
    if ( !objectsForIndirectExecute.empty() ) {
        auto device = m_pOwner->GetDevice();
        std::vector<InstanceData> instanceData;
        datas.clear();

        for (const auto& obj : objectsForIndirectExecute) {
            auto objIndirectDatas = obj->GetDataToExecute();
            for (auto [data, instance] : objIndirectDatas) {
                if (m_xCullingType == CullingType::CPUFrustum && !m_pObjectCulling->CheckFrustumCulling(frustum, instance)) continue;

                datas.push_back(data);
                instance.CommandIndex = (UINT)(datas.size() - 1);
                instanceData.push_back(instance);
            }
        }

        auto dataSize = (UINT)datas.size();
        m_pIndirectDeferredFirstPassCommandsBuffer->UploadData(device, list, datas.data(), dataSize, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        
        if (m_xCullingType==CullingType::GPUCulling) {
            InputObjectCullingProcessData data;
            data.CommandList = list;
            data.Device = device;
            data.DepthResource = m_pOwner->GetDepthResource();
            data.InputCommandsBuffer = m_pIndirectDeferredFirstPassCommandsBuffer.get();
            data.Instances = instanceData;
            data.CameraFrustum = frustum;
            m_pObjectCulling->ProcessGPUCulling(data);
        }

        ID3D12DescriptorHeap* heaps[] = { GlobalTextureHeap::GetInstance()->GetResult()->GetDescriptorPtr() };
        list->SetDescriptorHeaps(_countof(heaps), heaps);
        list->SetGraphicsRootDescriptorTable(TEXTURE_START_POSITION_IN_ROOT_SIG, GlobalTextureHeap::GetInstance()->GetResult()->GetGPUDescriptorHandle(0));

        if (m_xCullingType == CullingType::GPUCulling) {
            auto cullingRes = m_pObjectCulling->GetResultBuffer();
            cullingRes->ResourceBarrierChange(list, 1, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
            list->ExecuteIndirect(m_pIndirectDeferredFirstPassCommandSignature.Get(), dataSize, cullingRes->GetResource(), 0, cullingRes->GetResource(), m_pObjectCulling->CountBufferOffset((UINT)instanceData.size()));
        }
        else {
            m_pIndirectDeferredFirstPassCommandsBuffer->ResourceBarrierChange(list, 1, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
            list->ExecuteIndirect(m_pIndirectDeferredFirstPassCommandSignature.Get(), dataSize, m_pIndirectDeferredFirstPassCommandsBuffer->GetResource(), 0, nullptr, 0);
        }
    }

    PSOManager::GetInstance()->GetPSOObject(PSOType::DefferedFirstPassDefaultConfig)->Bind(list);
    for (const auto& obj : objectsForDefaultRender) {
        obj->DeferredRender(list);
    }
}

void MainRenderer_RenderableObjectsManager::InitIndirectDeferredMeshExecution() {
    D3D12_INDIRECT_ARGUMENT_DESC argumentDescs[6] = {};
    argumentDescs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW;
    argumentDescs[0].ConstantBufferView.RootParameterIndex = CONSTANT_BUFFER_MATRICES_POSITION_IN_ROOT_SIG;
    argumentDescs[1].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW;
    argumentDescs[1].ConstantBufferView.RootParameterIndex = CONSTANT_BUFFER_MATERIALS_POSITION_IN_ROOT_SIG;
    argumentDescs[2].Type = D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW;
    argumentDescs[2].ShaderResourceView.RootParameterIndex = ANIMATIONS_CONSTANT_BUFFER_IN_ROOT_SIG;
    argumentDescs[3].Type = D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW;
    argumentDescs[3].VertexBuffer.Slot = 0;
    argumentDescs[4].Type = D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW;
    argumentDescs[5].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

    D3D12_COMMAND_SIGNATURE_DESC commandSignatureDesc = {};
    commandSignatureDesc.pArgumentDescs = argumentDescs;
    commandSignatureDesc.NumArgumentDescs = _countof(argumentDescs);
    commandSignatureDesc.ByteStride = sizeof(IndirectMeshRenderableData);

    auto rootSignature = PSOManager::GetInstance()->GetPSOObject(PSOType::DefferedFirstPassDefaultConfig)->GetRootSignature();
    HRESULT_ASSERT(m_pOwner->GetDevice()->CreateCommandSignature(&commandSignatureDesc, rootSignature, IID_PPV_ARGS(m_pIndirectDeferredFirstPassCommandSignature.ReleaseAndGetAddressOf())), "Incorrect creation of Indirect Command Signature");

    m_pIndirectDeferredFirstPassCommandsBuffer = FD3DW::StructuredBuffer::CreateStructuredBuffer<IndirectMeshRenderableData>(m_pOwner->GetDevice(), 1u, true);

    m_pObjectCulling = std::make_unique<ObjectCulling>( m_pOwner->GetDevice() );
}

void MainRenderer_RenderableObjectsManager::DoDeleteObject(BaseRenderableObject* obj) {
    auto it = std::remove_if(m_vObjects.begin(), m_vObjects.end(),
        [obj](const std::unique_ptr<BaseRenderableObject>& o) {
            return o.get() == obj;
        });

    if (it != m_vObjects.end()) {
        m_vObjects.erase(it, m_vObjects.end());
    }

    m_bIsNeedUpdateTLAS = true;
}
