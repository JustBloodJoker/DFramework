#include <Entity/Camera/DefaultCameraInputLayer.h>
#include <Component/Camera/CameraComponent.h>
#include <Entity/Core/ComponentHolder.h>
#include <World/World.h>
#include <Entity/Camera/TDefaultCamera.h>

DefaultCameraInputLayer::DefaultCameraInputLayer(TDefaultCamera* cameraOwner) {
	m_pCameraOwner = cameraOwner;
}

bool DefaultCameraInputLayer::ProcessInput(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	auto isInput = false;
	switch (msg)
	{
	case WM_KEYDOWN:
		if (wParam == 'R')
		{
			m_pCameraOwner->ResetRoll();
		}
		isInput = true;
		break;

	case WM_MOUSEMOVE:
		m_pCameraOwner->OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		isInput = true;
		break;
	}

	return isInput;
}

void DefaultCameraInputLayer::PreTickUpdate(float DT) {
	if (IsKeyDown('W')) m_pCameraOwner->MoveForward(DT);
	if (IsKeyDown('S')) m_pCameraOwner->MoveBackward(DT);
	if (IsKeyDown('D')) m_pCameraOwner->StrafeRight(DT);
	if (IsKeyDown('A')) m_pCameraOwner->StrafeLeft(DT);
}
