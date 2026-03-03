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
	std::shared_ptr<FD3DW::ExecutionHandle> OnEndTick(std::shared_ptr<FD3DW::ExecutionHandle> handle);

	virtual void ProcessNotify(NRenderSystemNotifyType type) override;

	void OnResizeWindow();

	dx::XMFLOAT2 GetPrevJitterOffset();
	dx::XMFLOAT2 GetJitterOffset();
	dx::XMMATRIX GetPrevViewProjectionMatrix();
	dx::XMMATRIX GetViewProjectionMatrix();
	dx::XMMATRIX GetJitteredViewProjectionMatrix();
	dx::XMMATRIX GetJitteredProjectionMatrix();
	dx::XMMATRIX GetProjectionMatrix();
	dx::XMMATRIX GetViewMatrix();
	dx::XMFLOAT3 GetCameraPosition();

	void UpdateProjectionMatrix();
	float GetFoVY() const;
	CameraFrustum GetCameraFrustum();
	void UpdateCameraFrustum();

	CameraComponent* GetActiveComponent();

protected:
	float Halton(int index, int base);

protected:
	dx::XMFLOAT2 m_xJitterOffset;
	dx::XMFLOAT2 m_xPrevJitterOffset;

	dx::XMMATRIX m_xPrevViewProjectionMatrix;
	dx::XMMATRIX m_xProjectionMatrix;

	bool m_bIsEnabledJitter = true;
	dx::XMMATRIX m_xJitteredProjectionMatrix;

protected:


	CameraComponent* m_pCurrentActiveCamera = nullptr;
	CameraFrustum m_xFrustum;
	float m_fZNear = 0.1f;
	float m_fZFar = 10000.0f;

	std::atomic<bool> m_bIsNeedUpdateCamera{ false };
	std::atomic<bool> m_bIsNeedCheckCamera{ false };


	std::unique_ptr<CameraSystemInputLayer> m_pCameraLayer = nullptr;
};
