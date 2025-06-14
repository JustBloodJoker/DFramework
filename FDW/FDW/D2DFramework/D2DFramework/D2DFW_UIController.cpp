#include "D2DFW_UIController.h"

namespace FD2DW {

	void D2DFW_UIController::AddUIElement(std::shared_ptr<UIElementBase> element) {
        m_vUIElements.push_back(element);
    }

    void D2DFW_UIController::D2DUILoop() {
        auto rtv = GetRenderTarget();
        rtv->Clear(D2D1::ColorF(D2D1::ColorF::Black));

        for (auto& elem : m_vUIElements) {
            if (elem) elem->Draw(rtv, GetBrush());
        }
    }

    void D2DFW_UIController::ProcessInput(UINT msg, WPARAM wParam, LPARAM lParam) {
        for (auto& elem : m_vUIElements) {
            if (!elem) continue;

            auto elemInput = std::dynamic_pointer_cast<UIElementBaseWithInput>(elem);
            if (elemInput && elemInput->OnInput(msg, wParam, lParam))
                break;
        }
    }
}