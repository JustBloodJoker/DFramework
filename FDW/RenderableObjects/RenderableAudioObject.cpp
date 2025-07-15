#include <RenderableObjects/RenderableAudioObject.h>

RenderableAudioObject::RenderableAudioObject(std::unique_ptr<FD3DW::Audio> audio, const std::string& path) : BaseRenderableObject(path){
	m_pAudio = std::move(audio);
}

void RenderableAudioObject::Init(ID3D12Device* device, ID3D12GraphicsCommandList* list) {}

void RenderableAudioObject::BeforeRender(const BeforeRenderInputData& data) {
	TickAudioUpdate();
}

void RenderableAudioObject::DeferredRender(ID3D12GraphicsCommandList* list) {}

void RenderableAudioObject::ForwardRender(ID3D12GraphicsCommandList* list) {}

RenderPass RenderableAudioObject::GetRenderPass() const {
	return RenderPass::None;
}

void RenderableAudioObject::Play() {
	m_pAudio->Play();
}
void RenderableAudioObject::Stop() {
	m_pAudio->Stop();
}

void RenderableAudioObject::Restart() {
	m_pAudio->Restart();
}

void RenderableAudioObject::SetVolume(float volume) {
	m_pAudio->SetVolume(volume);
}
float RenderableAudioObject::GetVolume() {
	return m_pAudio->GetVolume();
}

bool RenderableAudioObject::IsLoop() {
	return m_bIsLoop;
}

void RenderableAudioObject::Loop(bool loop) {
	m_bIsLoop = loop;
}

void RenderableAudioObject::TickAudioUpdate() {
	if ((m_bIsLoop != m_pAudio->IsLoop()) && m_pAudio->IsEnded()) {
		m_pAudio->Restart();

		if (!m_bIsLoop) m_pAudio->Stop();
	}
}