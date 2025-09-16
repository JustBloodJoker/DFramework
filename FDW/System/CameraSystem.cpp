#include <System/CameraSystem.h>
#include <MainRenderer/MainRenderer.h>
#include <Component/Camera/CameraComponent.h>

dx::XMMATRIX GetDummyViewMatrix() {
	dx::XMVECTOR eye = dx::XMVectorSet(0.0f, 0.0f, -5.0f, 0.0f);
	dx::XMVECTOR target = dx::XMVectorZero();
	dx::XMVECTOR up = dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	return XMMatrixLookAtLH(eye, target, up);
}

dx::XMFLOAT3 GetDummyViewPosition() {
	dx::XMFLOAT3 pos;
	auto invView = dx::XMMatrixInverse(nullptr, GetDummyViewMatrix());
	auto cameraPosition = invView.r[3];
	XMStoreFloat3(&pos, cameraPosition);
	return pos;
}

void CameraSystem::AfterConstruction() {
	m_pCameraLayer = std::make_unique<CameraSystemInputLayer>(this);
	m_pCameraLayer->AddToRouter(m_pOwner->GetInputRouter());
	UpdateProjectionMatrix();
}

void CameraSystem::BeforeDestruction() {
	m_pCameraLayer->AddToRouter(nullptr);
}

dx::XMMATRIX CameraSystem::GetProjectionMatrix() {
	return m_xProjectionMatrix;
}

dx::XMMATRIX CameraSystem::GetViewMatrix() {
	return  m_pCurrentActiveCamera ? m_pCurrentActiveCamera->GetViewMatrix() : GetDummyViewMatrix();
}

dx::XMFLOAT3 CameraSystem::GetCameraPosition() {
	return m_pCurrentActiveCamera ? m_pCurrentActiveCamera->GetCameraPosition() : GetDummyViewPosition();
}

void CameraSystem::UpdateProjectionMatrix() {
	const auto& WndSet = m_pOwner->WNDSettings();
	m_xProjectionMatrix = dx::XMMatrixPerspectiveFovLH(M_PI_2_F, (float)WndSet.Width / WndSet.Height, 0.1f, 10000.0f);

	UpdateCameraFrustum();
}

void CameraSystem::OnResizeWindow() {
	UpdateProjectionMatrix();
}

CameraFrustum CameraSystem::GetCameraFrustum() {
	return m_xFrustum;
}

void CameraSystem::UpdateCameraFrustum() {
	if (!m_pCurrentActiveCamera) return;

	m_xFrustum.UpdatePlanes(GetViewMatrix() * m_xProjectionMatrix, m_fZNear, m_fZFar);
}

CameraComponent* CameraSystem::GetActiveComponent() {
	return m_pCurrentActiveCamera;
}

std::shared_ptr<FD3DW::ExecutionHandle> CameraSystem::OnStartTick(std::shared_ptr<FD3DW::ExecutionHandle> handle) {
	return GlobalRenderThreadManager::GetInstance()->SubmitLambda([this]() {

		if (m_bIsNeedCheckCamera.exchange(false, std::memory_order_acq_rel)) {
			auto components = GetWorld()->GetAllComponentsOfType<CameraComponent>();

			m_pCurrentActiveCamera = nullptr;
			for (const auto& cmp : components) {
				if (cmp->IsActive()) {
					m_pCurrentActiveCamera = cmp;
					m_bIsNeedUpdateCamera.store(true, std::memory_order_relaxed);
					break;
				}
			}
		}

		if (m_bIsNeedUpdateCamera.exchange(false, std::memory_order_acq_rel)) {
			UpdateCameraFrustum();
		}

	}, { handle }, true);
}

void CameraSystem::ProcessNotify(NRenderSystemNotifyType type) {
	if (type == NRenderSystemNotifyType::CameraActivationDeactivation) {
		m_bIsNeedCheckCamera.store(true, std::memory_order_relaxed);
	}
	else if (type == NRenderSystemNotifyType::CameraInfoChanged) {
		m_bIsNeedUpdateCamera.store(true, std::memory_order_relaxed);
	}
}
