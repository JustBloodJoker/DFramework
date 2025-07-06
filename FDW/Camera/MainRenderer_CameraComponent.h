#pragma once

#include <pch.h>
#include <MainRenderer/MainRendererComponent.h>
#include <Camera/CameraInputLayer.h>

class MainRenderer;

class MainRenderer_CameraComponent : virtual public MainRendererComponent {
public:
	MainRenderer_CameraComponent(MainRenderer* owner);
	virtual ~MainRenderer_CameraComponent() = default;

public:
	virtual void AfterConstruction();
	virtual void BeforeDestruction();
	void InitDefault();

	dx::XMMATRIX GetProjectionMatrix() const;
	dx::XMMATRIX GetViewMatrix() const;
	void UpdateViewMatrix();
	void UpdateProjectionMatrix();


public:
	void OnResizeWindow();
	void OnKeyInput(WPARAM wParam);
	void OnMouseMove(WPARAM btnState, int x, int y);

private:
	dx::XMMATRIX m_xView;
	dx::XMMATRIX m_xProjectionMatrix;

	float m_fCamYaw;
	float m_fCamPitch;
	float m_fCamRoll;
	POINT m_xLastMousePos;
	dx::XMVECTOR m_xEye;
	dx::XMVECTOR m_xUp;
	dx::XMVECTOR m_xStartUp;
	dx::XMVECTOR m_xAt;

private:
	std::unique_ptr<CameraInputLayer> m_pCameraLayer = nullptr;
};
