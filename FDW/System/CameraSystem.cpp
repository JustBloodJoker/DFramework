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

	m_xJitterOffset = { 0.0f, 0.0f };
	m_xPrevJitterOffset = { 0.0f, 0.0f };
	m_xJitteredProjectionMatrix = m_xProjectionMatrix;
	m_xPrevViewProjectionMatrix = GetViewProjectionMatrix();
}

void CameraSystem::BeforeDestruction() {
	m_pCameraLayer->AddToRouter(nullptr);
}

dx::XMFLOAT2 CameraSystem::GetPrevJitterOffset() {
	return m_xPrevJitterOffset;
}

dx::XMFLOAT2 CameraSystem::GetJitterOffset() {
	return m_xJitterOffset;
}

dx::XMMATRIX CameraSystem::GetPrevViewProjectionMatrix() {
	return m_xPrevViewProjectionMatrix;
}

dx::XMMATRIX CameraSystem::GetViewProjectionMatrix() {
	return GetViewMatrix() * GetProjectionMatrix();
}

dx::XMMATRIX CameraSystem::GetJitteredViewProjectionMatrix() {
	return GetViewMatrix() * GetJitteredProjectionMatrix();
}

dx::XMMATRIX CameraSystem::GetJitteredProjectionMatrix() {
	return m_xJitteredProjectionMatrix;
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
	m_xProjectionMatrix = dx::XMMatrixPerspectiveFovLH(GetFoVY(), (float)WndSet.Width / WndSet.Height, m_fZNear, m_fZFar);

	UpdateCameraFrustum();
}

float CameraSystem::GetFoVY() const {
	return M_PI_2_F;
}

void CameraSystem::OnResizeWindow() {
	UpdateProjectionMatrix();
}

CameraFrustum CameraSystem::GetCameraFrustum() {
	return m_xFrustum;
}

void CameraSystem::UpdateCameraFrustum() {
	if (!m_pCurrentActiveCamera) return;

	m_xFrustum.UpdatePlanes(GetViewProjectionMatrix(), m_fZNear, m_fZFar);
}

CameraComponent* CameraSystem::GetActiveComponent() {
	return m_pCurrentActiveCamera;
}

float CameraSystem::Halton(int index, int base) {
	auto f = 1.0f;
	auto result = 0.0f;
	auto current = index;

	while (current > 0) {
		f = f / (float)base;
		result = result + f * (float)(current % base);
		current = (int)(current / (float)base);
	}
	return result;
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

		m_xJitteredProjectionMatrix = m_xProjectionMatrix;
		m_xJitterOffset = { 0.0f, 0.0f };
		if (!m_bIsEnabledJitter) return;

		auto frameIdx = m_pOwner->GetFrameIndex();
		const auto& wndSet = m_pOwner->WNDSettings();

		auto jitterIndex = frameIdx % TAA_JITTER_PHASES_NUM;

		auto haltonX = Halton(jitterIndex + 1, 2) - 0.5f;
		auto haltonY = Halton(jitterIndex + 1, 3) - 0.5f;

		auto jitterScale = 1.0f;
		if (auto timer = m_pOwner->GetTimer(); timer) {
			auto targetFrameTime = 1.0f / TAA_PREFER_FRAME_RATE_PHASES_NUM;
			auto dt = (float)timer->GetDeltaTime();
			if (dt > targetFrameTime && dt > 0.0001f) {
				jitterScale = targetFrameTime / dt;

				if (jitterScale > 1.0f) jitterScale = 1.0f;

				jitterScale *= jitterScale;

				if (jitterScale < 0.05f) jitterScale = 0.0f;
			}
		}
		
		m_xJitterOffset.x = ((haltonX * 2.0f) / (float)wndSet.Width) * jitterScale;
		m_xJitterOffset.y = ((haltonY * 2.0f) / (float)wndSet.Height) * jitterScale;



		m_xJitteredProjectionMatrix.r[2] = dx::XMVectorAdd(m_xJitteredProjectionMatrix.r[2], dx::XMVectorSet(m_xJitterOffset.x, m_xJitterOffset.y, 0.0f, 0.0f));

	}, { handle }, true, true);
}

std::shared_ptr<FD3DW::ExecutionHandle> CameraSystem::OnEndTick(std::shared_ptr<FD3DW::ExecutionHandle> handle) {
	return GlobalRenderThreadManager::GetInstance()->SubmitLambda([this]() {
		m_xPrevViewProjectionMatrix = GetViewProjectionMatrix();
		m_xPrevJitterOffset = m_xJitterOffset;
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
