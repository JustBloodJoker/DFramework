#include "pch.h"

#include <System/CameraFrustum.h>

void CameraFrustum::UpdateDepthRange(float zNear, float zFar) {
    m_fZNear = zNear;
    m_fZFar = zFar;
}

float CameraFrustum::GetZNear() {
    return m_fZNear;
}

float CameraFrustum::GetZFar() {
    return m_fZFar;
}
