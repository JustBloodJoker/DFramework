#pragma once

#include <pch.h>
#include <Entity/Camera/TBaseCamera.h>
#include <Entity/Camera/DefaultCameraInputLayer.h>


class TDefaultCamera : public TBaseCamera {
public:
	TDefaultCamera();
	virtual ~TDefaultCamera() = default;

	virtual void ResetPosition() override;

	bool IsActiveCameraComponent();

public:
    REFLECT_BODY(TDefaultCamera)
    BEGIN_REFLECT(TDefaultCamera, TBaseCamera)
        REFLECT_PROPERTY(m_fCamYaw)
        REFLECT_PROPERTY(m_fCamPitch)
        REFLECT_PROPERTY(m_fCamRoll)
        REFLECT_PROPERTY(m_xEye)
        REFLECT_PROPERTY(m_xUp)
        REFLECT_PROPERTY(m_xAt)
    END_REFLECT(TDefaultCamera)


public:
	void MoveForward(float dt);
	void MoveBackward(float dt);
	void StrafeLeft(float dt);
	void StrafeRight(float dt);
	void ResetRoll();

	void OnMouseMove(WPARAM btnState, int x, int y);


	virtual void Init() override;
	virtual void Destroy() override;
	
	void UpdateCamera();


protected:
	float m_fCamYaw;
	float m_fCamPitch;
	float m_fCamRoll;
	POINT m_xLastMousePos;

	dx::XMVECTOR m_xEye;
	dx::XMVECTOR m_xUp;
	dx::XMVECTOR m_xAt;

private:
	std::unique_ptr<DefaultCameraInputLayer> m_pCameraLayer = nullptr;
};