#include "pch.h"

#include <Component/Camera/CameraComponent.h>
#include <World/World.h>


CameraComponent::CameraComponent() {
	m_sName = "CameraComponent";

	m_xEye = dx::XMVectorSet(0.0f, 2.0f, -1.0f, 1.0f);
	m_xAt = dx::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	m_xUp = dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	InitDefault();
}

void CameraComponent::Init() {
	GetWorld()->AddNotifyToPull(NRenderSystemNotifyType::CameraActivationDeactivation);
}

void CameraComponent::Activate(bool a) {
	IComponent::Activate(a);
	GetWorld()->AddNotifyToPull(NRenderSystemNotifyType::CameraActivationDeactivation);
}

void CameraComponent::SetAllVectors(dx::XMVECTOR newEye, dx::XMVECTOR newUp, dx::XMVECTOR newAt) {
	m_xEye = newEye;
	m_xUp = newUp;
	m_xAt = newAt;
	UpdateViewMatrix();
}

void CameraComponent::InitDefault() {
	UpdateViewMatrix();
}

dx::XMMATRIX CameraComponent::GetViewMatrix() const {
	return m_xView;
}

dx::XMFLOAT3 CameraComponent::GetCameraPosition() const
{
	dx::XMFLOAT3 pos;
	auto invView = dx::XMMatrixInverse(nullptr, m_xView);
	auto cameraPosition = invView.r[3];
	XMStoreFloat3(&pos, cameraPosition);
	return pos;
}


void CameraComponent::UpdateViewMatrix() {
	m_xView = dx::XMMatrixLookAtLH(m_xEye, m_xAt, m_xUp);
	if(auto world = GetWorld())world->AddNotifyToPull(NRenderSystemNotifyType::CameraInfoChanged);
}
