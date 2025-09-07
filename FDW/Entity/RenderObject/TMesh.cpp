#include <Entity/RenderObject/TMesh.h>
#include <World/World.h>



void TMesh::SetPosition(const dx::XMFLOAT3& pos) {
    m_xPosition = pos;

    ForEachComponentOfType<MeshComponent>([this](MeshComponent* comp) { 
        comp->SetParentPosition(m_xPosition); 
    });
}

void TMesh::SetRotation(const dx::XMFLOAT3& rot) {
    m_xRotation = rot;

    ForEachComponentOfType<MeshComponent>([this](MeshComponent* comp) { 
        comp->SetParentRotation(m_xRotation);
    });
}

void TMesh::SetScale(const dx::XMFLOAT3& scale) {
    m_xScaling = scale;

    ForEachComponentOfType<MeshComponent>([this](MeshComponent* comp) {
        comp->SetParentScale(m_xScaling);
    });
}


dx::XMFLOAT3 TMesh::GetPosition() const { return m_xPosition; }
dx::XMFLOAT3 TMesh::GetRotation() const { return m_xRotation; }
dx::XMFLOAT3 TMesh::GetScale()    const { return m_xScaling; }


void TMesh::DoRenderInit(ID3D12Device* device, ID3D12GraphicsCommandList* list) {
    std::shared_ptr<FD3DW::ExecutionHandle> syncOnInitParent = nullptr;
    if (!m_bIsCreated) {
        BeforeRenderInitAfterCreation(device, list);
        m_bIsCreated = true;
    }
    else
    {
        BeforeRenderInitAfterLoad(device, list);
    }

    TRender::DoRenderInit(device, list);
}
