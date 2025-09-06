#pragma once

#include <pch.h>
#include <MainRenderer/MainRendererComponent.h>
#include <System/CameraFrustum.h>
#include <System/CameraSystemInputLayer.h>
#include <MainRenderer/GlobalRenderThreadManager.h>

class CameraComponent;

class CameraSystem : public MainRendererComponent {
public:
	CameraSystem() = default;
	virtual ~CameraSystem() = default;

	virtual void AfterConstruction() override;
	virtual void BeforeDestruction() override;

	std::shared_ptr<FD3DW::ExecutionHandle> OnStartTick(std::shared_ptr<FD3DW::ExecutionHandle> handle);

	virtual void ProcessNotify(NRenderSystemNotifyType type) override;

	void OnResizeWindow();

	dx::XMMATRIX GetProjectionMatrix();
	dx::XMMATRIX GetViewMatrix();
	dx::XMFLOAT3 GetCameraPosition();

	void UpdateProjectionMatrix();
	CameraFrustum GetCameraFrustum();
	void UpdateCameraFrustum();

	CameraComponent* GetActiveComponent();
protected:

	dx::XMMATRIX m_xProjectionMatrix;
	CameraComponent* m_pCurrentActiveCamera = nullptr;
	CameraFrustum m_xFrustum;
	float m_fZNear = 0.1f;
	float m_fZFar = 10000.0f;

	std::atomic<bool> m_bIsNeedUpdateCamera{ false };
	std::atomic<bool> m_bIsNeedCheckCamera{ false };


	std::unique_ptr<CameraSystemInputLayer> m_pCameraLayer = nullptr;
};
