#include "pch.h"

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

void TBaseCamera::ResetPosition() {
	if (!m_pCameraComponent) return;

	m_pCameraComponent->SetAllVectors(m_xStartEye, m_xStartUp, m_xStartAt);
}
