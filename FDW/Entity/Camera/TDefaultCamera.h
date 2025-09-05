#pragma once

#include <pch.h>
#include <Entity/Camera/TBaseCamera.h>
#include <Entity/Camera/DefaultCameraInputLayer.h>


class TDefaultCamera : public TBaseCamera {
public:
	TDefaultCamera();
	virtual ~TDefaultCamera() = default;

	virtual void ResetPosition() override;


public:
	BEGIN_FIELD_REGISTRATION(TDefaultCamera, TBaseCamera)
		REGISTER_FIELD(m_fCamYaw);
		REGISTER_FIELD(m_fCamPitch);
		REGISTER_FIELD(m_fCamRoll);
	END_FIELD_REGISTRATION();


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

private:
	std::unique_ptr<DefaultCameraInputLayer> m_pCameraLayer = nullptr;
};