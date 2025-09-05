#pragma once

#include <pch.h>
#include <MainRenderer/MainRendererComponent.h>
#include <Camera/CameraInputLayer.h>
#include <System/CameraFrustum.h>


class MainRenderer;

class MainRenderer_CameraComponent : virtual public MainRendererComponent {
public:
	MainRenderer_CameraComponent();
	virtual ~MainRenderer_CameraComponent() = default;

public:
	virtual void AfterConstruction() override;
	virtual void BeforeDestruction() override;
	void InitDefault();

	dx::XMMATRIX GetProjectionMatrix() const;
	dx::XMMATRIX GetViewMatrix() const;
	dx::XMFLOAT3 GetCameraPosition() const;
	void UpdateViewMatrix();
	void UpdateProjectionMatrix();

public:
	void ResetPosition();
	float GetCameraSpeed() const;
	void SetCameraSpeed(float speed);

public:
	void OnResizeWindow();

	void MoveForward(float dt);
	void MoveBackward(float dt);
	void StrafeLeft(float dt);
	void StrafeRight(float dt);
	void ResetRoll();
	
	void OnMouseMove(WPARAM btnState, int x, int y);

	CameraFrustum GetCameraFrustum();


public:


	BEGIN_FIELD_REGISTRATION(MainRenderer_CameraComponent, MainRendererComponent)
		REGISTER_FIELD(m_fCameraSpeed)
		REGISTER_FIELD(m_fCamYaw)
		REGISTER_FIELD(m_fCamPitch)
		REGISTER_FIELD(m_fCamRoll)
		REGISTER_FIELD(m_xEye)
		REGISTER_FIELD(m_xUp)
		REGISTER_FIELD(m_xAt)
		REGISTER_FIELD(m_xStartUp)
		REGISTER_FIELD(m_xStartEye)
		REGISTER_FIELD(m_xStartAt)
		REGISTER_FIELD(m_fZFar)
		REGISTER_FIELD(m_fZNear)
	END_FIELD_REGISTRATION()

private:
	void UpdateCameraFrustum();

private:
	dx::XMMATRIX m_xProjectionMatrix;
	dx::XMMATRIX m_xView;

	CameraFrustum m_xFrustum;

	float m_fCameraSpeed = 1000.0f;
	float m_fCamYaw;
	float m_fCamPitch;
	float m_fCamRoll;
	POINT m_xLastMousePos;
	dx::XMVECTOR m_xEye;
	dx::XMVECTOR m_xUp;
	dx::XMVECTOR m_xAt;

	float m_fZNear = 0.1f;
	float m_fZFar = 10000.0f;

	dx::XMVECTOR m_xStartUp;
	dx::XMVECTOR m_xStartEye = dx::XMVectorZero();
	dx::XMVECTOR m_xStartAt = dx::XMVectorZero();

private:
	std::unique_ptr<CameraInputLayer> m_pCameraLayer = nullptr;
};
