#pragma once

#include <pch.h>
#include <D3DFramework/Utilites/Serializer/ReflectionImpl.h>

class CameraFrustum {
public:
	CameraFrustum() = default;

	void UpdatePlanes(const dx::XMMATRIX& vp, const float zNear, const float zFar);

	std::array<dx::XMFLOAT4, 6> GetPlanes();
	dx::XMMATRIX GetViewProjection();
	float GetZNear();
	float GetZFar();

	bool IsAABBInsideFrustum(const dx::XMFLOAT3& boxMin, const dx::XMFLOAT3& boxMax);

protected:

	float m_fZNear;
	float m_fZFar;
	dx::XMMATRIX m_xViewProjection;
	std::array<dx::XMFLOAT4, 6> m_aPlanes;
};