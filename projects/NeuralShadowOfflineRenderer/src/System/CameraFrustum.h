#pragma once

#include <WinWindow/Utils/Reflection/Reflection.h>
#include <DirectXMath.h>

namespace dx = DirectX;

class CameraFrustum {
public:
	CameraFrustum() = default;

	void UpdateDepthRange(float zNear, float zFar);
	float GetZNear();
	float GetZFar();

protected:

	float m_fZNear;
	float m_fZFar;
};
