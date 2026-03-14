#pragma once

#include <pch.h>
#include <WinWindow/InputSystem/WinWindowInputLayer.h>

class CameraSystem;

class CameraSystemInputLayer : virtual public FDWWIN::WinWindowInputLayer {
public:
	CameraSystemInputLayer(CameraSystem* cameraOwner);

	virtual bool ProcessInput(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	
private:
	CameraSystem* m_pCameraOwner = nullptr;

};