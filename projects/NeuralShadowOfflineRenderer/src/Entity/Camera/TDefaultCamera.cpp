#include "pch.h"

#include <Entity/Camera/TDefaultCamera.h>
#include <World/World.h>
#include <Renderer/OfflineRenderer.h>


TDefaultCamera::TDefaultCamera() {
	m_sName = "DefaultCamera";

	m_xStartUp = dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	m_xStartEye = dx::XMVectorSet(0.0f, 2.0f, -1.0f, 1.0f);
	m_xStartAt = dx::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

	m_xEye = m_xStartEye;
	m_xAt = m_xStartAt;
	m_fCamYaw = 0.0f;
	m_fCamPitch = 0.0f;
	m_fCamRoll = 0.0f;
}
void TDefaultCamera::SetEyeAndLookAt(float eyeX, float eyeY, float eyeZ, float lookX, float lookY, float lookZ) {
	m_xEye = dx::XMVectorSet(eyeX, eyeY, eyeZ, 1.0f);

	auto lookAt = dx::XMVectorSet(lookX, lookY, lookZ, 1.0f);
	auto dir = lookAt - m_xEye;
	auto dirLenSq = dx::XMVectorGetX(dx::XMVector3LengthSq(dir));
	if (dirLenSq <= 1e-6f) {
		dir = dx::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	}
	else {
		dir = dx::XMVector3Normalize(dir);
	}

	dx::XMFLOAT3 dir3;
	dx::XMStoreFloat3(&dir3, dir);
	m_fCamYaw = std::atan2(dir3.x, dir3.z);
	m_fCamPitch = -std::asin(std::clamp(dir3.y, -1.0f, 1.0f));

	m_xAt = lookAt;
	m_xUp = dx::XMVector3Normalize(dx::XMVector3TransformCoord(
		m_xStartUp, dx::XMMatrixRotationRollPitchYaw(0, 0, m_fCamRoll)));
	if (m_pCameraComponent) m_pCameraComponent->SetAllVectors(m_xEye, m_xUp, m_xAt);
}

void TDefaultCamera::ResetPosition() {
	TBaseCamera::ResetPosition();

	m_fCamPitch = 0.0f;
	m_fCamYaw = 0.0f;
	m_fCamRoll = 0.0f;

	m_xEye = m_xStartEye;
	m_xAt = m_xStartAt;
}
