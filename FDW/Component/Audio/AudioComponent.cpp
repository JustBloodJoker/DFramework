#include <Component/Audio/AudioComponent.h>
#include <World/World.h>
#include <MainRenderer/MainRenderer.h>

AudioComponent::AudioComponent(const std::string& path) : m_sPath(path) {
    m_sName = m_sPath;
}

void AudioComponent::Init() {
    auto manager = GetWorld()->GetMainRenderer()->GetAudioMananger();
    m_pAudio.reset(manager->CreateAudio(m_sPath));
    GetWorld()->AddNotifyToPull(NRenderSystemNotifyType::AudioActivationDeactivation);
}

void AudioComponent::Play() { 
    if (m_pAudio) {
        m_pAudio->Play();
        Activate(true);
    }
}
void AudioComponent::Stop() { 
    if (m_pAudio) {
        m_pAudio->Stop();
        Activate(false);
    }
}
void AudioComponent::Restart() { 
    if (m_pAudio) {
        m_pAudio->Restart();
        Activate(true);
    }
}

void AudioComponent::SetVolume(float volume) { 
    if (m_pAudio) m_pAudio->SetVolume(volume); 
}

float AudioComponent::GetVolume() const { return m_pAudio ? m_pAudio->GetVolume() : 0.0f; }

bool AudioComponent::IsLoop() const { return m_bIsLoop; }

void AudioComponent::Loop(bool loop) { 
    m_bIsLoop = loop;
}

void AudioComponent::AudioTick() {
    if (m_pAudio && (m_bIsLoop != m_pAudio->IsLoop()) && m_pAudio->IsEnded()) {
        m_pAudio->Restart();
        if (!m_bIsLoop) m_pAudio->Stop();
    }
}

void AudioComponent::Activate(bool a) {
    auto prevAct = IsActive();
    IComponent::Activate(a);
    GetWorld()->AddNotifyToPull(NRenderSystemNotifyType::AudioActivationDeactivation);

    if (prevAct == IsActive()) return;

    if (IsActive()) {
        Play();
    }
    else {
        Stop();
    }
}

void AudioComponent::Destroy() {
    Stop();
    GetWorld()->AddNotifyToPull(NRenderSystemNotifyType::AudioActivationDeactivation);
}
