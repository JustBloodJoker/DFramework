#pragma once

#include <Renderer/OfflineRendererComponent.h>
#include <System/CameraFrustum.h>
#include <Renderer/GlobalRenderThreadManager.h>

class CameraComponent;

class CameraSystem : public OfflineRendererComponent {
public:
	CameraSystem() = default;
	virtual ~CameraSystem() = default;

	virtual void AfterConstruction() override;

	std::shared_ptr<FD3DW::ExecutionHandle> OnStartTick(std::shared_ptr<FD3DW::ExecutionHandle> handle);

	virtual void ProcessNotify(NRenderSystemNotifyType type) override;

	dx::XMMATRIX GetViewProjectionMatrix();
	dx::XMMATRIX GetProjectionMatrix();
	dx::XMMATRIX GetViewMatrix();
	dx::XMFLOAT3 GetCameraPosition();

	void UpdateProjectionMatrix();
	void SetProjectionParameters(float fovY, float zNear, float zFar);
	float GetFoVY() const;
	float GetZNear() const;
	float GetZFar() const;
	CameraFrustum GetCameraFrustum();
	void UpdateCameraFrustum();

protected:
	static dx::XMMATRIX GetDummyViewMatrix();
	static dx::XMFLOAT3 GetDummyViewPosition();

protected:
	dx::XMMATRIX m_xProjectionMatrix;

protected:
	CameraComponent* m_pCurrentActiveCamera = nullptr;
	CameraFrustum m_xFrustum;
	float m_fZNear = 0.1f;
	float m_fZFar = 10000.0f;
	float m_fFoVY = M_PI_2_F;

	std::atomic<bool> m_bIsNeedUpdateCamera{ false };
	std::atomic<bool> m_bIsNeedCheckCamera{ false };

};
