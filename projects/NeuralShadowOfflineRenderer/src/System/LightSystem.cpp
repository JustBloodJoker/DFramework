#include "pch.h"

#include <Renderer/OfflineRenderer.h>
#include <System/LightSystem.h>
#include <Component/Light/LightComponent.h>


void LightSystem::AfterConstruction() {
    m_pLightsStructuredBuffer = FD3DW::StructuredBuffer::CreateStructuredBuffer<LightComponentData>(m_pOwner->GetDevice(), 1u, true);

}

int LightSystem::GetLightsCount() {
    return static_cast<int>(m_vLightComponentsData.size());
}

FD3DW::StructuredBuffer* LightSystem::GetLightsBuffer() {
    return m_pLightsStructuredBuffer.get();
}

const std::vector<LightComponentData>& LightSystem::GetLightComponentsData() const {
    return m_vLightComponentsData;
}

std::shared_ptr<FD3DW::ExecutionHandle> LightSystem::OnStartRenderTick(std::shared_ptr<FD3DW::ExecutionHandle> syncHandle) {
    auto updateRecipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_DIRECT,
        [this](ID3D12GraphicsCommandList* list) mutable {
            auto components = GetWorld()->GetAllComponentsOfType<LightComponent>();

            if (m_bIsNeedUpdateDataInBuffer.exchange(false, std::memory_order_acq_rel)) {
                auto device = m_pOwner->GetDevice();
                m_vLightComponentsData = GetDataFromLightComponents(components);
                m_pLightsStructuredBuffer->UploadData(
					device,
					list,
					m_vLightComponentsData.data(),
					static_cast<int>(m_vLightComponentsData.size()),
					D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
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
