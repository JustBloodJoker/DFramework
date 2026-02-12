#include <MainRenderer/MainRenderer.h>
#include <System/LightSystem.h>
#include <Component/Light/LightComponent.h>


void LightSystem::AfterConstruction() {
    MainRendererComponent::AfterConstruction();

    m_pLightsHelperConstantBuffer = FD3DW::UploadBuffer< LightSystemBuffer>::CreateConstantBuffer(m_pOwner->GetDevice(), 1u);
    m_pLightsStructuredBuffer = FD3DW::StructuredBuffer::CreateStructuredBuffer<LightComponentData>(m_pOwner->GetDevice(), 1u, true);
}

int LightSystem::GetLightsCount() {
    return m_xLightBuffer.LightCount;
}

FD3DW::StructuredBuffer* LightSystem::GetLightsBuffer() {
    return m_pLightsStructuredBuffer.get();
}

D3D12_GPU_VIRTUAL_ADDRESS LightSystem::GetLightsStructuredBufferGPULocation() {
    return m_pLightsStructuredBuffer->GetResource()->GetGPUVirtualAddress();
}

D3D12_GPU_VIRTUAL_ADDRESS LightSystem::GetLightsConstantBufferGPULocation() {
    return m_pLightsHelperConstantBuffer->GetGPULocation(0);
}

const std::vector<LightComponentData>& LightSystem::GetLightComponentsData() const { 
    return m_vLightComponentsData; 
}

std::shared_ptr<FD3DW::ExecutionHandle> LightSystem::OnStartRenderTick(std::shared_ptr<FD3DW::ExecutionHandle> syncHandle) {
    auto updateRecipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_DIRECT, 
        [this](ID3D12GraphicsCommandList* list) mutable {
            auto components = GetWorld()->GetAllComponentsOfType<LightComponent>();

            m_xLightBuffer.CameraPos = m_pOwner->GetCurrentCameraPosition();
            m_xLightBuffer.IsShadowImpl = (int)m_pOwner->IsShadowEnabled();
            m_xLightBuffer.ZNear = m_pOwner->GetCameraFrustum().GetZNear();
            m_xLightBuffer.ZFar = m_pOwner->GetCameraFrustum().GetZFar();

            m_pLightsHelperConstantBuffer->CpyData(0, m_xLightBuffer);

            if (m_bIsNeedUpdateDataInBuffer.exchange(false, std::memory_order_acq_rel)) {
                auto device = m_pOwner->GetDevice();
                m_vLightComponentsData = GetDataFromLightComponents(components);
                m_xLightBuffer.LightCount = int(m_vLightComponentsData.size());
                m_pLightsStructuredBuffer->UploadData(device, list, m_vLightComponentsData.data(), m_xLightBuffer.LightCount, D3D12_RESOURCE_STATE_COPY_DEST);
            }
    });
    
    return GlobalRenderThreadManager::GetInstance()->Submit(updateRecipe, { syncHandle }, true);
}

void LightSystem::ProcessNotify(NRenderSystemNotifyType type) {
    if (type == NRenderSystemNotifyType::Light || type == NRenderSystemNotifyType::LightUpdateData) {
        m_bIsNeedUpdateDataInBuffer.store(true, std::memory_order_relaxed);
    }
}

std::vector<LightComponentData> LightSystem::GetDataFromLightComponents(std::vector<LightComponent*> cmps) {
    std::vector<LightComponentData> ret;
    for (const auto& cmp : cmps) {
        if( cmp->IsActive() ) ret.push_back(cmp->GetLightComponentData());
    }
    return ret;
}
