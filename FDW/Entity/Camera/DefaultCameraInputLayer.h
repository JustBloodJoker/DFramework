#pragma once

#include <pch.h>
#include <WinWindow/InputSystem/WinWindowInputLayer.h>

class TDefaultCamera;

class DefaultCameraInputLayer : virtual public FDWWIN::WinWindowInputLayer {
public:
	DefaultCameraInputLayer(TDefaultCamera* cameraOwner);

	virtual bool ProcessInput(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	virtual void PreTickUpdate(float DT) override;

private:
	TDefaultCamera* m_pCameraOwner = nullptr;

};