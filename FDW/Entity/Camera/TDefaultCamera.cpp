#include <Entity/Camera/TDefaultCamera.h>
#include <World/World.h>
#include <MainRenderer/MainRenderer.h>


TDefaultCamera::TDefaultCamera() {
	m_sName = "DefaultCamera"; 
	
	m_xStartUp = dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	m_xStartEye = dx::XMVectorSet(0.0f, 2.0f, -1.0f, 1.0f);
	m_xStartAt = dx::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

	m_xEye = m_xStartEye;
	m_xAt = m_xStartAt;
}
void TDefaultCamera::MoveForward(float dt) {
	dx::XMVECTOR dir = dx::XMVector3Normalize(m_xAt - m_xEye);
	m_xEye = dx::XMVectorAdd(m_xEye, dx::XMVectorScale(dir, m_fCameraSpeed * dt));
	UpdateCamera();
}

void TDefaultCamera::MoveBackward(float dt) {
	dx::XMVECTOR dir = dx::XMVector3Normalize(m_xAt - m_xEye);
	m_xEye = dx::XMVectorSubtract(m_xEye, dx::XMVectorScale(dir, m_fCameraSpeed * dt));
	UpdateCamera();
}

void TDefaultCamera::StrafeRight(float dt) {
	dx::XMVECTOR dir = dx::XMVector3Normalize(m_xAt - m_xEye);
	dx::XMVECTOR right = dx::XMVector3Normalize(dx::XMVector3Cross(m_xUp, dir));
	m_xEye = dx::XMVectorAdd(m_xEye, dx::XMVectorScale(right, m_fCameraSpeed * dt));
	UpdateCamera();
}

void TDefaultCamera::StrafeLeft(float dt) {
	dx::XMVECTOR dir = dx::XMVector3Normalize(m_xAt - m_xEye);
	dx::XMVECTOR right = dx::XMVector3Normalize(dx::XMVector3Cross(m_xUp, dir));
	m_xEye = dx::XMVectorSubtract(m_xEye, dx::XMVectorScale(right, m_fCameraSpeed * dt));
	UpdateCamera();
}

void TDefaultCamera::ResetRoll() {
	m_fCamRoll = 0.0f;
	UpdateCamera();
}


void TDefaultCamera::OnMouseMove(WPARAM btnState, int x, int y) {
	bool isChanged = false;
	if ((btnState & MK_LBUTTON) != 0 && ((x != m_xLastMousePos.x) || (y != m_xLastMousePos.y)))
	{
		m_fCamYaw += (m_xLastMousePos.x - x) * 0.002f;
		m_fCamPitch += (m_xLastMousePos.y - y) * 0.002f;
		isChanged = true;
	}
	if ((btnState & MK_RBUTTON) != 0 && ((x != m_xLastMousePos.x) || (y != m_xLastMousePos.y)))
	{
		m_fCamRoll += (m_xLastMousePos.x - x) * 0.002f;
		isChanged = true;
	}
	m_xLastMousePos.x = x;
	m_xLastMousePos.y = y;

	if(isChanged) UpdateCamera();
}

void TDefaultCamera::Init() {
	TBaseCamera::Init();

	m_pCameraLayer = std::make_unique<DefaultCameraInputLayer>(this);
	m_pCameraLayer->AddToRouter(GetWorld()->GetMainRenderer()->GetInputRouter());
}

void TDefaultCamera::Destroy() {
	TBaseCamera::Destroy();

	m_pCameraLayer->AddToRouter(nullptr);
}

void TDefaultCamera::UpdateCamera() {

	m_xAt = dx::XMVectorAdd(dx::XMVector3Normalize(dx::XMVector3TransformCoord(dx::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f),
		dx::XMMatrixRotationRollPitchYaw(m_fCamPitch, m_fCamYaw, 0))), m_xEye);

	m_xUp = dx::XMVector3Normalize(dx::XMVector3TransformCoord(m_xStartUp, dx::XMMatrixRotationRollPitchYaw(0, 0, m_fCamRoll)));
	
	if (!m_pCameraComponent) return;

	m_pCameraComponent->SetAllVectors(m_xEye, m_xUp, m_xAt);
}


void TDefaultCamera::ResetPosition() {
	TBaseCamera::ResetPosition();

	m_fCamPitch = 0.0f;
	m_fCamYaw = 0.0f;
	m_fCamRoll = 0.0f;

	m_xEye = m_xStartEye;
	m_xAt = m_xStartAt;
}

bool TDefaultCamera::IsActiveCameraComponent() {
	auto cmp = GetComponent<CameraComponent>();
	return cmp ? cmp->IsActive() : false;
}
