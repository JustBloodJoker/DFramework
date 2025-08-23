#include <Camera/MainRenderer_CameraComponent.h>
#include <MainRenderer/MainRenderer.h>

MainRenderer_CameraComponent::MainRenderer_CameraComponent() {
	m_xEye = dx::XMVectorSet(0.0f, 2.0f, -1.0f, 1.0f);
	m_xAt = dx::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	m_xStartUp = dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	m_xStartEye = m_xEye;
	m_xStartAt = m_xAt;
}

void MainRenderer_CameraComponent::AfterConstruction() {
	InitDefault();

	m_pCameraLayer = std::make_unique<CameraInputLayer>(this);
	m_pCameraLayer->AddToRouter(m_pOwner->GetInputRouter());
}

void MainRenderer_CameraComponent::BeforeDestruction() {
	m_pCameraLayer->AddToRouter(nullptr);
}

void MainRenderer_CameraComponent::InitDefault() {


	UpdateViewMatrix();
	UpdateProjectionMatrix();
}

dx::XMMATRIX MainRenderer_CameraComponent::GetProjectionMatrix() const {
	return m_xProjectionMatrix;
}

dx::XMMATRIX MainRenderer_CameraComponent::GetViewMatrix() const {
	return m_xView;
}

dx::XMFLOAT3 MainRenderer_CameraComponent::GetCameraPosition() const
{
	dx::XMFLOAT3 pos;
	auto invView = dx::XMMatrixInverse(nullptr, m_xView);
	auto cameraPosition = invView.r[3];
	XMStoreFloat3(&pos, cameraPosition);
	return pos;
}

void MainRenderer_CameraComponent::OnResizeWindow() {
	UpdateProjectionMatrix();
}

void MainRenderer_CameraComponent::MoveForward(float dt) {
	dx::XMVECTOR dir = dx::XMVector3Normalize(m_xAt - m_xEye);
	m_xEye = dx::XMVectorAdd(m_xEye, dx::XMVectorScale(dir, m_fCameraSpeed * dt));
	UpdateViewMatrix();
}

void MainRenderer_CameraComponent::MoveBackward(float dt) {
	dx::XMVECTOR dir = dx::XMVector3Normalize(m_xAt - m_xEye);
	m_xEye = dx::XMVectorSubtract(m_xEye, dx::XMVectorScale(dir, m_fCameraSpeed * dt));
	UpdateViewMatrix();
}

void MainRenderer_CameraComponent::StrafeRight(float dt) {
	dx::XMVECTOR dir = dx::XMVector3Normalize(m_xAt - m_xEye);
	dx::XMVECTOR right = dx::XMVector3Normalize(dx::XMVector3Cross(m_xUp, dir));
	m_xEye = dx::XMVectorAdd(m_xEye, dx::XMVectorScale(right, m_fCameraSpeed * dt));
	UpdateViewMatrix();
}

void MainRenderer_CameraComponent::StrafeLeft(float dt) {
	dx::XMVECTOR dir = dx::XMVector3Normalize(m_xAt - m_xEye);
	dx::XMVECTOR right = dx::XMVector3Normalize(dx::XMVector3Cross(m_xUp, dir));
	m_xEye = dx::XMVectorSubtract(m_xEye, dx::XMVectorScale(right, m_fCameraSpeed * dt));
	UpdateViewMatrix();
}

void MainRenderer_CameraComponent::ResetRoll() {
	m_fCamRoll = 0.0f;
	UpdateViewMatrix();
}

void MainRenderer_CameraComponent::ResetPosition() {
	m_xEye = m_xStartEye;
	m_xAt = m_xStartAt;
	m_fCamPitch = 0.0f;
	m_fCamYaw = 0.0f;
	m_fCamRoll = 0.0f;
	UpdateViewMatrix();
}

float MainRenderer_CameraComponent::GetCameraSpeed() const {
	return m_fCameraSpeed;
}

void MainRenderer_CameraComponent::SetCameraSpeed(float speed) {
	m_fCameraSpeed = speed;
}

void MainRenderer_CameraComponent::OnMouseMove(WPARAM btnState, int x, int y) {
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

	UpdateViewMatrix();
}
	
CameraFrustum MainRenderer_CameraComponent::GetCameraFrustum() {
	return m_xFrustum;
}

void MainRenderer_CameraComponent::UpdateCameraFrustum() {
	m_xFrustum.UpdatePlanes(m_xView * m_xProjectionMatrix, m_fZNear, m_fZFar);
}


void MainRenderer_CameraComponent::UpdateViewMatrix() {
	m_xAt = dx::XMVectorAdd(dx::XMVector3Normalize(dx::XMVector3TransformCoord(dx::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f),
		dx::XMMatrixRotationRollPitchYaw(m_fCamPitch, m_fCamYaw, 0))), m_xEye);

	m_xUp = dx::XMVector3Normalize(dx::XMVector3TransformCoord(m_xStartUp, dx::XMMatrixRotationRollPitchYaw(0, 0, m_fCamRoll)));
	m_xView = dx::XMMatrixLookAtLH(m_xEye, m_xAt, m_xUp);

	UpdateCameraFrustum();
}

void MainRenderer_CameraComponent::UpdateProjectionMatrix() {
	const auto& WndSet = m_pOwner->WNDSettings();
	m_xProjectionMatrix = dx::XMMatrixPerspectiveFovLH(M_PI_2_F, (float)WndSet.Width / WndSet.Height, 0.1f, 10000.0f);

	UpdateCameraFrustum();
}
