#include "pch.h"

#include <System/CameraSystem.h>
#include <Renderer/OfflineRenderer.h>
#include <Component/Camera/CameraComponent.h>

dx::XMMATRIX CameraSystem::GetDummyViewMatrix() {
	dx::XMVECTOR eye = dx::XMVectorSet(0.0f, 0.0f, -5.0f, 0.0f);
	dx::XMVECTOR target = dx::XMVectorZero();
	dx::XMVECTOR up = dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	return XMMatrixLookAtLH(eye, target, up);
}

dx::XMFLOAT3 CameraSystem::GetDummyViewPosition() {
	dx::XMFLOAT3 pos;

	auto invView = dx::XMMatrixInverse(nullptr, GetDummyViewMatrix());
	auto cameraPosition = invView.r[3];
	
	XMStoreFloat3(&pos, cameraPosition);
	return pos;
}

void CameraSystem::AfterConstruction() {
	UpdateProjectionMatrix();
}

dx::XMMATRIX CameraSystem::GetViewProjectionMatrix() {
	return GetViewMatrix() * GetProjectionMatrix();
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
	auto sceneWidth = std::max(1, m_pOwner->GetSceneRenderWidth());
	auto sceneHeight = std::max(1, m_pOwner->GetSceneRenderHeight());
	m_xProjectionMatrix = dx::XMMatrixPerspectiveFovLH(GetFoVY(), (float)sceneWidth / (float)sceneHeight, m_fZNear, m_fZFar);

	UpdateCameraFrustum();
}

float CameraSystem::GetFoVY() const {
	return m_fFoVY;
}

void CameraSystem::SetProjectionParameters(float fovY, float zNear, float zFar) {
	if (!std::isfinite(fovY) || !std::isfinite(zNear) || !std::isfinite(zFar) || fovY <= 0.0f || fovY >= M_PI_F || zNear <= 0.0f || zFar <= zNear) return;
	
	m_fFoVY = fovY;
	m_fZNear = zNear;
	m_fZFar = zFar;
	
	UpdateProjectionMatrix();
}

float CameraSystem::GetZNear() const {
	return m_fZNear;
}

float CameraSystem::GetZFar() const {
	return m_fZFar;
}

CameraFrustum CameraSystem::GetCameraFrustum() {
	return m_xFrustum;
}

void CameraSystem::UpdateCameraFrustum() {
	if (!m_pCurrentActiveCamera) return;

	m_xFrustum.UpdateDepthRange(m_fZNear, m_fZFar);
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

	}, { handle }, true, true);
}

void CameraSystem::ProcessNotify(NRenderSystemNotifyType type) {
	if (type == NRenderSystemNotifyType::CameraActivationDeactivation) {
		m_bIsNeedCheckCamera.store(true, std::memory_order_relaxed);
	}
	else if (type == NRenderSystemNotifyType::CameraInfoChanged) {
		m_bIsNeedUpdateCamera.store(true, std::memory_order_relaxed);
	}
}
