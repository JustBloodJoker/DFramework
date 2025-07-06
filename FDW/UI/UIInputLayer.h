#pragma once

#include <pch.h>
#include <WinWindow/InputSystem/WinWindowInputLayer.h>

class MainRenderer_UIComponent;

class UIInputLayer : virtual public FDWWIN::WinWindowInputLayer {
public:
	UIInputLayer(MainRenderer_UIComponent* uiOwner);

	virtual bool ProcessInput(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;

private:
	MainRenderer_UIComponent* m_pUIOwner = nullptr;

};