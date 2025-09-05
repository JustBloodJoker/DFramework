#include <Entity/Audio/AudioComponent.h>
#include <World/World.h>
#include <MainRenderer/MainRenderer.h>


AudioComponent::AudioComponent(const std::string& path) : m_sPath(path) {
    m_sName = m_sPath;
}

void AudioComponent::Init() {
    auto manager = GetWorld()->GetMainRenderer()->GetAudioMananger();
    m_pAudio.reset(manager->CreateAudio(m_sPath));
    GetWorld()->AddNotifyToPull(NRenderSystemNotifyType::Audio);
}

void AudioComponent::Play() { 
    if (m_pAudio) m_pAudio->Play(); 
    GetWorld()->AddNotifyToPull(NRenderSystemNotifyType::Audio);
}
void AudioComponent::Stop() { 
    if (m_pAudio) m_pAudio->Stop();
    GetWorld()->AddNotifyToPull(NRenderSystemNotifyType::Audio);
}
void AudioComponent::Restart() { 
    if (m_pAudio) m_pAudio->Restart(); 
    GetWorld()->AddNotifyToPull(NRenderSystemNotifyType::Audio);
}

void AudioComponent::SetVolume(float volume) { 
    if (m_pAudio) m_pAudio->SetVolume(volume); 
    GetWorld()->AddNotifyToPull(NRenderSystemNotifyType::Audio);
}

float AudioComponent::GetVolume() const { return m_pAudio ? m_pAudio->GetVolume() : 0.0f; }

bool AudioComponent::IsLoop() const { return m_bIsLoop; }

void AudioComponent::Loop(bool loop) { 
    m_bIsLoop = loop; 
    GetWorld()->AddNotifyToPull(NRenderSystemNotifyType::Audio);
}

void AudioComponent::BeforeRenderTick(float dt) {
    if (m_pAudio && (m_bIsLoop != m_pAudio->IsLoop()) && m_pAudio->IsEnded()) {
        m_pAudio->Restart();
        if (!m_bIsLoop) m_pAudio->Stop();
    }
}

void AudioComponent::Destroy() {
    Stop();
    GetWorld()->AddNotifyToPull(NRenderSystemNotifyType::Audio);
}
