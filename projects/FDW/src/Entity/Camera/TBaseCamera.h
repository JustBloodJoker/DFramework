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
    REFLECT_BODY(TBaseCamera)
    BEGIN_REFLECT(TBaseCamera, ComponentHolder)
        REFLECT_PROPERTY(m_fCameraSpeed)
        REFLECT_PROPERTY(m_pCameraComponent)
        REFLECT_PROPERTY(m_xStartEye)
        REFLECT_PROPERTY(m_xStartUp)
        REFLECT_PROPERTY(m_xStartAt)
    END_REFLECT(TBaseCamera)
	
protected:
	virtual void OnComponentRemoved(IComponent* comp) override;

protected:
	float m_fCameraSpeed = 100.0f;

	CameraComponent* m_pCameraComponent = nullptr;

	dx::XMVECTOR m_xStartEye;
	dx::XMVECTOR m_xStartUp;
	dx::XMVECTOR m_xStartAt;
};
