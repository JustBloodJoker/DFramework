#include <Camera/CameraInputLayer.h>
#include <Camera/MainRenderer_CameraComponent.h>

CameraInputLayer::CameraInputLayer(MainRenderer_CameraComponent* cameraOwner) {
	m_pCameraOwner = cameraOwner;
}

bool CameraInputLayer::ProcessInput(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	auto isInput = false;
	switch (msg)
	{
		case WM_SIZE:
			m_pCameraOwner->OnResizeWindow();
			break;

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

void CameraInputLayer::PreTickUpdate(float dt) {
	if (IsKeyDown('W')) m_pCameraOwner->MoveForward(dt);
	if (IsKeyDown('S')) m_pCameraOwner->MoveBackward(dt);
	if (IsKeyDown('D')) m_pCameraOwner->StrafeRight(dt);
	if (IsKeyDown('A')) m_pCameraOwner->StrafeLeft(dt);
}
