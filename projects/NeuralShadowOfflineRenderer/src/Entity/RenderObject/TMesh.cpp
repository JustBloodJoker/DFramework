#include "pch.h"

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


void TMesh::DoRenderInit(ID3D12Device* device, ID3D12GraphicsCommandList* list) {
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
