#include <Entity/Camera/TDefaultCamera.h>
#include <World/World.h>
#include <MainRenderer/MainRenderer.h>


TDefaultCamera::TDefaultCamera() {
	m_sName = "DefaultCamera"; 
	
	m_xStartUp = dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	m_xStartEye = dx::XMVectorSet(0.0f, 2.0f, -1.0f, 1.0f);
	m_xStartAt = dx::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
}

void TDefaultCamera::MoveForward(float dt) {
	if (!m_pCameraComponent) return;

	auto at = m_pCameraComponent->GetAt();
	auto eye = m_pCameraComponent->GetEye();
	
	dx::XMVECTOR dir = dx::XMVector3Normalize(at - eye);
	eye = dx::XMVectorAdd(eye, dx::XMVectorScale(dir, m_fCameraSpeed * dt));
	m_pCameraComponent->SetEye(eye);
}

void TDefaultCamera::MoveBackward(float dt) {
	if (!m_pCameraComponent) return;

	auto at = m_pCameraComponent->GetAt();
	auto eye = m_pCameraComponent->GetEye();


	dx::XMVECTOR dir = dx::XMVector3Normalize(at - eye);
	eye = dx::XMVectorSubtract(eye, dx::XMVectorScale(dir, m_fCameraSpeed * dt));
	m_pCameraComponent->SetEye(eye);
}

void TDefaultCamera::StrafeRight(float dt) {
	if (!m_pCameraComponent) return;

	auto at = m_pCameraComponent->GetAt();
	auto eye = m_pCameraComponent->GetEye();
	auto up = m_pCameraComponent->GetUp();

	dx::XMVECTOR dir = dx::XMVector3Normalize(at - eye);
	dx::XMVECTOR right = dx::XMVector3Normalize(dx::XMVector3Cross(up, dir));
	eye = dx::XMVectorAdd(eye, dx::XMVectorScale(right, m_fCameraSpeed * dt));
	m_pCameraComponent->SetEye(eye);
}

void TDefaultCamera::StrafeLeft(float dt) {
	if (!m_pCameraComponent) return;

	auto at = m_pCameraComponent->GetAt();
	auto eye = m_pCameraComponent->GetEye();
	auto up = m_pCameraComponent->GetUp();

	dx::XMVECTOR dir = dx::XMVector3Normalize(at - eye);
	dx::XMVECTOR right = dx::XMVector3Normalize(dx::XMVector3Cross(up, dir));
	eye = dx::XMVectorSubtract(eye, dx::XMVectorScale(right, m_fCameraSpeed * dt));
	m_pCameraComponent->SetEye(eye);
}

void TDefaultCamera::ResetRoll() {
	m_fCamRoll = 0.0f;
	UpdateCamera();
}


void TDefaultCamera::OnMouseMove(WPARAM btnState, int x, int y) {
	if ((btnState & MK_LBUTTON) != 0 && ((x != m_xLastMousePos.x) || (y != m_xLastMousePos.y)))
	{
		m_fCamYaw += (m_xLastMousePos.x - x) * 0.002f;
		m_fCamPitch += (m_xLastMousePos.y - y) * 0.002f;
	}
	if ((btnState & MK_RBUTTON) != 0 && ((x != m_xLastMousePos.x) || (y != m_xLastMousePos.y)))
	{
		m_fCamRoll += (m_xLastMousePos.x - x) * 0.002f;
	}
	m_xLastMousePos.x = x;
	m_xLastMousePos.y = y;

	UpdateCamera();
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
	if (!m_pCameraComponent) return;
	
	auto eye = m_pCameraComponent->GetEye();
	auto at = dx::XMVectorAdd(dx::XMVector3Normalize(dx::XMVector3TransformCoord(dx::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f),
		dx::XMMatrixRotationRollPitchYaw(m_fCamPitch, m_fCamYaw, 0))), eye);

	auto up = dx::XMVector3Normalize(dx::XMVector3TransformCoord(m_xStartUp, dx::XMMatrixRotationRollPitchYaw(0, 0, m_fCamRoll)));
	
	m_pCameraComponent->SetEye(eye);
	m_pCameraComponent->SetAt(at);
	m_pCameraComponent->SetUp(up);
}

void TDefaultCamera::ResetPosition() {
	TBaseCamera::ResetPosition();

	m_fCamPitch = 0.0f;
	m_fCamYaw = 0.0f;
	m_fCamRoll = 0.0f;
}
//todo input layer for every camera 