#include <Entity/Camera/TBaseCamera.h>
#include <World/World.h>

void TBaseCamera::AfterCreation() {
	ComponentHolder::AfterCreation();

	m_pCameraComponent = AddComponent<CameraComponent>();
	ResetPosition();
}

void TBaseCamera::Activate(bool act) {
	if (m_pCameraComponent) m_pCameraComponent->Activate(act);
}

bool TBaseCamera::IsActive() {
	return m_pCameraComponent ? m_pCameraComponent->IsActive() : false;
}

void TBaseCamera::SetCameraSpeed(float speed) {
	m_fCameraSpeed = speed;
}

float TBaseCamera::GetCameraSpeed() const {
	return m_fCameraSpeed;
}

void TBaseCamera::ResetPosition() {
	if (!m_pCameraComponent) return;
	
	m_pCameraComponent->SetAllVectors(m_xStartEye, m_xStartUp, m_xStartAt);
}

void TBaseCamera::OnComponentRemoved(IComponent* comp) {
	ComponentHolder::OnComponentRemoved(comp);

	if (m_pCameraComponent == comp) m_pCameraComponent = nullptr;
}
