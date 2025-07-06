#include <Camera/MainRenderer_CameraComponent.h>
#include <MainRenderer/MainRenderer.h>

MainRenderer_CameraComponent::MainRenderer_CameraComponent(MainRenderer* owner) :MainRendererComponent(owner) {}

void MainRenderer_CameraComponent::AfterConstruction() {
	InitDefault();

	m_pCameraLayer = std::make_unique<CameraInputLayer>(this);
	m_pCameraLayer->AddToRouter(m_pOwner->GetInputRouter());
}

void MainRenderer_CameraComponent::BeforeDestruction() {
	m_pCameraLayer->AddToRouter(nullptr);
}

void MainRenderer_CameraComponent::InitDefault() {
	m_xEye = dx::XMVectorSet(0.0f, 0.0f, -1.0f, 1.0f);
	m_xAt = dx::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	m_xStartUp = dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	UpdateViewMatrix();
	UpdateProjectionMatrix();
}

dx::XMMATRIX MainRenderer_CameraComponent::GetProjectionMatrix() const {
	return m_xProjectionMatrix;
}

dx::XMMATRIX MainRenderer_CameraComponent::GetViewMatrix() const {
	return m_xView;
}

void MainRenderer_CameraComponent::OnResizeWindow() {
	UpdateProjectionMatrix();
}

void MainRenderer_CameraComponent::OnKeyInput(WPARAM wParam) {
	dx::XMVECTOR cameraDirection = dx::XMVector3Normalize(dx::XMVectorSubtract(m_xAt, m_xEye));
	dx::XMVECTOR rightDirection = dx::XMVector3Normalize(dx::XMVector3Cross(m_xUp, cameraDirection));

	if (wParam == 'W')
	{
		m_xEye = dx::XMVectorAdd(m_xEye, dx::XMVectorScale(cameraDirection, 1000.0f * 0.016f));
	}
	if (wParam == 'S')
	{
		m_xEye = dx::XMVectorSubtract(m_xEye, dx::XMVectorScale(cameraDirection, 1000.0f * 0.016f));
	}
	if (wParam == 'D')
	{
		m_xEye = dx::XMVectorAdd(m_xEye, dx::XMVectorScale(rightDirection, 1000.0f * 0.016f));
	}
	if (wParam == 'A')
	{
		m_xEye = dx::XMVectorSubtract(m_xEye, dx::XMVectorScale(rightDirection, 1000.0f * 0.016f));
	}

	if (wParam == 'R')
	{
		m_fCamRoll = 0.0f;
	}

	UpdateViewMatrix();
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

void MainRenderer_CameraComponent::UpdateViewMatrix() {
	m_xAt = dx::XMVectorAdd(dx::XMVector3Normalize(dx::XMVector3TransformCoord(dx::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f),
		dx::XMMatrixRotationRollPitchYaw(m_fCamPitch, m_fCamYaw, 0))), m_xEye);

	m_xUp = dx::XMVector3Normalize(dx::XMVector3TransformCoord(m_xStartUp, dx::XMMatrixRotationRollPitchYaw(0, 0, m_fCamRoll)));
	m_xView = dx::XMMatrixLookAtLH(m_xEye, m_xAt, m_xUp);
}

void MainRenderer_CameraComponent::UpdateProjectionMatrix() {
	const auto& WndSet = m_pOwner->WNDSettings();
	m_xProjectionMatrix = dx::XMMatrixPerspectiveFovLH(M_PI_2_F, (float)WndSet.Width / WndSet.Height, 1.0f, 10000.0f);
}
