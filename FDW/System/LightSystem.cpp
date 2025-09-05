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

std::shared_ptr<FD3DW::ExecutionHandle> LightSystem::OnStartRenderTick(std::shared_ptr<FD3DW::ExecutionHandle> syncHandle) {
    auto updateRecipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_DIRECT, 
        [this](ID3D12GraphicsCommandList* list) mutable {
            auto components = GetWorld()->GetAllComponentsOfType<LightComponent>();

            m_xLightBuffer.CameraPos = m_pOwner->GetCurrentCameraPosition();
            m_xLightBuffer.IsShadowImpl = m_pOwner->CurrentShadowType() != ShadowType::None;

            m_pLightsHelperConstantBuffer->CpyData(0, m_xLightBuffer);

            if (m_bIsNeedUpdateDataInBuffer.exchange(false, std::memory_order_acq_rel)) {
                auto device = m_pOwner->GetDevice();
                auto data = GetDataFromLightComponents(components);
                m_xLightBuffer.LightCount = int(data.size());
                m_pLightsStructuredBuffer->UploadData(device, list, data.data(), UINT(data.size()), D3D12_RESOURCE_STATE_COPY_DEST);
            }
    });
    
    return GlobalRenderThreadManager::GetInstance()->Submit(updateRecipe, { syncHandle });
}

void LightSystem::ProcessNotify(NRenderSystemNotifyType type) {
    if (type == NRenderSystemNotifyType::Light) {
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
