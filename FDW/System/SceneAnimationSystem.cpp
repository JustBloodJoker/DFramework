#include <MainRenderer/MainRenderer.h>
#include <System/SceneAnimationSystem.h>


void SceneAnimationSystem::ProcessNotify(NRenderSystemNotifyType type) {
	if (type == NRenderSystemNotifyType::SceneAnimationActivationDeactivation) {
		m_bIsNeedUpdateActiveAnimations.store(true, std::memory_order_relaxed);
	}

}

std::shared_ptr<FD3DW::ExecutionHandle> SceneAnimationSystem::OnStartRenderTick(std::shared_ptr<FD3DW::ExecutionHandle> sync) {
	auto recipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_DIRECT, [this](ID3D12GraphicsCommandList* list) {
		if (m_bIsNeedUpdateActiveAnimations.exchange(false, std::memory_order_acq_rel)) {
			m_vActiveAnimations.clear();
			auto components = GetWorld()->GetAllComponentsOfType<AnimationComponent>();
			for (const auto& component : components) {
				if (component->IsActive()) {
					m_vActiveAnimations.push_back(component);
				}
			}
		}

		AnimationComponentInputData inData;
		inData.Device = m_pOwner->GetDevice();
		inData.CommandList = list;
		inData.DT = m_pOwner->GetTimer()->GetDeltaTime();

		for (auto* cmp : m_vActiveAnimations) {
			cmp->OnAnimationUpdateTick(inData);
		}
	});
	
	return GlobalRenderThreadManager::GetInstance()->Submit(recipe, { sync }, true);
}

std::shared_ptr<FD3DW::ExecutionHandle> SceneAnimationSystem::ProcessGPUSkinning(std::shared_ptr<FD3DW::ExecutionHandle> sync)
{
	auto recipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_COMPUTE, [this](ID3D12GraphicsCommandList* list) {
		for (auto* cmp : m_vActiveAnimations) {
			cmp->ProcessGPUSkinning(list);
		}
	});

	return GlobalRenderThreadManager::GetInstance()->Submit(recipe, { sync });
}

