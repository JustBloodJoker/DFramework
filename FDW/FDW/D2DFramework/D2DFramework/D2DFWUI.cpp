#include "D2DFWUI.h"

namespace FD2DW {

	void D2DFWUI::AddUIElement(std::shared_ptr<UIElementBase> element) {
        m_vUIElements.push_back(element);
    }

    void D2DFWUI::UserD2DLoop() {
        auto rtv = GetRenderTarget();
        rtv->Clear(D2D1::ColorF(D2D1::ColorF::Black));

        for (auto& elem : m_vUIElements) {
            if (elem) elem->Draw(rtv, GetBrush());
        }
    }
    void D2DFWUI::ProcessInput(UINT msg, WPARAM wParam, LPARAM lParam) {
        for (auto& elem : m_vUIElements) {
            if (!elem) continue;

            auto elemInput = std::dynamic_pointer_cast<UIElementBaseWithInput>(elem);
            if (elemInput && elemInput->OnInput(msg, wParam, lParam))
                break;
        }
    }
    void D2DFWUI::ChildMOUSEUP(WPARAM btnState, int x, int y) {
        ProcessInput(WM_LBUTTONUP, btnState, MAKELPARAM(x, y));
    }
    void D2DFWUI::ChildMOUSEDOWN(WPARAM btnState, int x, int y) {
        ProcessInput(WM_LBUTTONDOWN, btnState, MAKELPARAM(x, y));
    }
    void D2DFWUI::ChildMOUSEMOVE(WPARAM btnState, int x, int y) {
        ProcessInput(WM_MOUSEMOVE, btnState, MAKELPARAM(x, y));
    }
    void D2DFWUI::ChildKeyPressed(WPARAM key) {
        ProcessInput(WM_KEYDOWN, key, 0);
    }
}