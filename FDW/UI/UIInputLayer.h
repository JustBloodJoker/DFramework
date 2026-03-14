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

private:
	bool m_bLMBPendingSelection = false;
	bool m_bLMBDragDetected = false;
	int m_iLMBDownX = 0;
	int m_iLMBDownY = 0;
};
