#include <UI/UIInputLayer.h>
#include <UI/MainRenderer_UIComponent.h>

UIInputLayer::UIInputLayer(MainRenderer_UIComponent* uiOwner) {
	m_pUIOwner = uiOwner;
}

bool UIInputLayer::ProcessInput(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	m_pUIOwner->ImGuiInputProcess(hwnd, msg, wParam, lParam);

	if ((msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN) && wParam == VK_F1) {
		
		auto wasDown = (lParam & (1 << 30)) != 0;
		if (!wasDown) m_pUIOwner->ToggleUIVisible();

		return true;
	}

	if (msg == WM_LBUTTONDOWN) {
		m_iLMBDownX = GET_X_LPARAM(lParam);
		m_iLMBDownY = GET_Y_LPARAM(lParam);
		m_bLMBDragDetected = false;
		m_bLMBPendingSelection = m_pUIOwner->IsPointInsideScene(m_iLMBDownX, m_iLMBDownY);
		return false;
	}

	if (msg == WM_MOUSEMOVE && m_bLMBPendingSelection) {
		auto mouseX = GET_X_LPARAM(lParam);
		auto mouseY = GET_Y_LPARAM(lParam);
		auto deltaX = std::abs(mouseX - m_iLMBDownX);
		auto deltaY = std::abs(mouseY - m_iLMBDownY);
		
		if (std::max(deltaX, deltaY) > SELECTION_DRAG_THRESHOLD_PIXELS) {
			m_bLMBDragDetected = true;
		}

		return false;
	}

	if (msg == WM_LBUTTONUP) {
		auto mouseX = GET_X_LPARAM(lParam);
		auto mouseY = GET_Y_LPARAM(lParam);
		bool canSelect = m_bLMBPendingSelection && !m_bLMBDragDetected && m_pUIOwner->IsPointInsideScene(mouseX, mouseY);
		m_bLMBPendingSelection = false;
		m_bLMBDragDetected = false;
		
		if (canSelect) m_pUIOwner->SelectEntityByWorldClick(mouseX, mouseY);

		return false;
	}

	if (msg == WM_KILLFOCUS) {
		m_bLMBPendingSelection = false;
		m_bLMBDragDetected = false;
	}

	return false;
}
