#pragma once

#include <pch.h>

class CameraFrustum {
public:
	CameraFrustum() = default;

	void UpdatePlanes(const dx::XMMATRIX& vp);
	std::array<dx::XMFLOAT4, 6> GetPlanes();

	bool IsAABBInsideFrustum(const dx::XMFLOAT3& boxMin, const dx::XMFLOAT3& boxMax);

protected:

	std::array<dx::XMFLOAT4, 6> m_aPlanes;
};