#include <UI/UIInputLayer.h>
#include <UI/MainRenderer_UIComponent.h>

UIInputLayer::UIInputLayer(MainRenderer_UIComponent* uiOwner) {
	m_pUIOwner = uiOwner;
}

bool UIInputLayer::ProcessInput(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	return m_pUIOwner->ImGuiInputProcess(hwnd, msg, wParam, lParam);
}
