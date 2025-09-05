#include <System/AudioSystem.h>
#include <World/World.h>

std::shared_ptr<FD3DW::ExecutionHandle> AudioSystem::OnStartTick(std::shared_ptr<FD3DW::ExecutionHandle> h) {
	return GlobalRenderThreadManager::GetInstance()->SubmitLambda([this]() {
		if (m_bIsNeedUpdateAudioActivation.exchange(false, std::memory_order_acq_rel)) {
			m_vActiveAudio.clear();
			auto cmps = GetWorld()->GetAllComponentsOfType<AudioComponent>();
			for (const auto& cmp : cmps) {
				if (cmp->IsActive())
				{
					m_vActiveAudio.push_back(cmp);
				}
			}
		}

		for (auto& activeCmp : m_vActiveAudio) {
			activeCmp->AudioTick();
		}
	});
}

void AudioSystem::ProcessNotify(NRenderSystemNotifyType type) {
	if (type == NRenderSystemNotifyType::AudioActivationDeactivation) {
		m_bIsNeedUpdateAudioActivation.store(true, std::memory_order_relaxed);
	}
}
