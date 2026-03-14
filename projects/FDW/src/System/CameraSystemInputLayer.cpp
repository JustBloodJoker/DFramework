#include <System/CameraSystemInputLayer.h>
#include <System/CameraSystem.h>
#include <Entity/Core/ComponentHolder.h>
#include <World/World.h>

CameraSystemInputLayer::CameraSystemInputLayer(CameraSystem* cameraOwner) {
	m_pCameraOwner = cameraOwner;
}

bool CameraSystemInputLayer::ProcessInput(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg)
	{
	case WM_SIZE:
		m_pCameraOwner->OnResizeWindow();
		break;
	}

	return false;

}
