#include <System/CameraFrustum.h>

void CameraFrustum::UpdatePlanes(const dx::XMMATRIX& vp, const float zNear, const float zFar) {
    m_xViewProjection = vp;
    m_fZNear = zNear;
    m_fZFar = zFar;

    dx::XMFLOAT4X4 m;
    dx::XMStoreFloat4x4(&m, vp);

    m_aPlanes[0] = dx::XMFLOAT4(m._14 + m._11, m._24 + m._21, m._34 + m._31, m._44 + m._41);
    m_aPlanes[1] = dx::XMFLOAT4(m._14 - m._11, m._24 - m._21, m._34 - m._31, m._44 - m._41);
    m_aPlanes[2] = dx::XMFLOAT4(m._14 + m._12, m._24 + m._22, m._34 + m._32, m._44 + m._42);
    m_aPlanes[3] = dx::XMFLOAT4(m._14 - m._12, m._24 - m._22, m._34 - m._32, m._44 - m._42);
    m_aPlanes[4] = dx::XMFLOAT4(m._13, m._23, m._33, m._43);
    m_aPlanes[5] = dx::XMFLOAT4(m._14 - m._13, m._24 - m._23, m._34 - m._33, m._44 - m._43);

    for (int i = 0; i < 6; i++) {
        dx::XMVECTOR p = dx::XMLoadFloat4(&m_aPlanes[i]);
        p = dx::XMPlaneNormalize(p);
        dx::XMStoreFloat4(&m_aPlanes[i], p);
    }
}

std::array<dx::XMFLOAT4, 6> CameraFrustum::GetPlanes() {
	return m_aPlanes;
}

dx::XMMATRIX CameraFrustum::GetViewProjection() {
    return m_xViewProjection;
}

float CameraFrustum::GetZNear() {
    return m_fZNear;
}

float CameraFrustum::GetZFar() {
    return m_fZFar;
}

bool CameraFrustum::IsAABBInsideFrustum(const dx::XMFLOAT3& boxMin, const dx::XMFLOAT3& boxMax) {
    for (int i = 0; i < 6; i++) {
        dx::XMFLOAT4 p = m_aPlanes[i];

        dx::XMFLOAT3 positive;
        positive.x = (p.x >= 0.0f) ? boxMax.x : boxMin.x;
        positive.y = (p.y >= 0.0f) ? boxMax.y : boxMin.y;
        positive.z = (p.z >= 0.0f) ? boxMax.z : boxMin.z;

        float dist = p.x * positive.x + p.y * positive.y + p.z * positive.z + p.w;

        if (dist < 0.0f) return false;
    }

    return true;
}
