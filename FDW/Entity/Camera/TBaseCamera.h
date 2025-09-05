#pragma once

#include <pch.h>
#include <Entity/Core/ComponentHolder.h>
#include <Component/Camera/CameraComponent.h>

class TBaseCamera : public ComponentHolder {
public:
	TBaseCamera() = default;
	virtual ~TBaseCamera()=default;


	virtual void AfterCreation() override;

	void Activate(bool act);
	bool IsActive();

	void SetCameraSpeed(float speed);
	float GetCameraSpeed() const;

	virtual void ResetPosition();


public:
	BEGIN_FIELD_REGISTRATION(TBaseCamera, ComponentHolder)
		REGISTER_FIELD(m_fCameraSpeed);
		REGISTER_FIELD(m_pCameraComponent);
		REGISTER_FIELD(m_xStartEye);
		REGISTER_FIELD(m_xStartUp);
		REGISTER_FIELD(m_xStartAt);
	END_FIELD_REGISTRATION();
	
protected:
	virtual void OnComponentRemoved(IComponent* comp) override;

protected:
	float m_fCameraSpeed = 1000.0f;

	CameraComponent* m_pCameraComponent = nullptr;

	dx::XMVECTOR m_xStartEye;
	dx::XMVECTOR m_xStartUp;
	dx::XMVECTOR m_xStartAt;
};
