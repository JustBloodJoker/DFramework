#include <Entity/RenderObject/TMesh.h>
#include <World/World.h>

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
