#pragma once

#include <pch.h>
#include <WinWindow/InputSystem/WinWindowInputLayer.h>

class MainRenderer_CameraComponent;

class CameraInputLayer : virtual public FDWWIN::WinWindowInputLayer {
public:
	CameraInputLayer(MainRenderer_CameraComponent* cameraOwner);

	virtual bool ProcessInput(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;

private:
	MainRenderer_CameraComponent* m_pCameraOwner = nullptr;

};