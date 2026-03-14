#include <Entity/Audio/TAudio.h>
#include <World/World.h>

TAudio::TAudio(std::string path) {
	m_sPath = path;
	m_sName = path;
}

void TAudio::Play() {
	for (auto cmp : GetComponents<AudioComponent>()) {
		cmp->Play();
	}
}

void TAudio::Stop() {
	for (auto cmp : GetComponents<AudioComponent>()) {
		cmp->Stop();
	}
}
void TAudio::Restart() {
	for (auto cmp : GetComponents<AudioComponent>()) {
		cmp->Restart();
	}
}

void TAudio::SetVolume(float volume) {
	for (auto cmp : GetComponents<AudioComponent>()) {
		cmp->SetVolume(volume);
	}
}
float TAudio::GetVolume() {
	auto cmp = GetComponent<AudioComponent>();
	return cmp ? cmp->GetVolume() : -1.0f;
}

bool TAudio::IsLoop() {
	for (auto cmp : GetComponents<AudioComponent>()) {
		if (cmp->IsLoop()) return true;
	}
	return false;
}
void TAudio::Loop(bool loop){
	for (auto cmp : GetComponents<AudioComponent>()) {
		cmp->Loop(loop);
	}
}

void TAudio::AfterCreation() {
	ComponentHolder::AfterCreation();

	if (!m_sPath.empty()) {
		AddComponent<AudioComponent>(m_sPath);
	}
}
