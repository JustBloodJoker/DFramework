#pragma once

#include <Entity/Camera/TBaseCamera.h>


class TDefaultCamera : public TBaseCamera {
public:
	TDefaultCamera();
	virtual ~TDefaultCamera() = default;

	virtual void ResetPosition() override;


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
	void SetEyeAndLookAt(float eyeX, float eyeY, float eyeZ, float lookX, float lookY, float lookZ);


protected:
	float m_fCamYaw;
	float m_fCamPitch;
	float m_fCamRoll;
	dx::XMVECTOR m_xEye;
	dx::XMVECTOR m_xUp;
	dx::XMVECTOR m_xAt;
};
