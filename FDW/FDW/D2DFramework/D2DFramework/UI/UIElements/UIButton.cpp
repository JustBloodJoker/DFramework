#include "UI/UIElements/UIButton.h"

namespace FD2DW {

    UIButton::UIButton(std::shared_ptr<BaseShape> shape) {
        SetShape(shape);
    }

    void UIButton::Draw(ID2D1RenderTarget* pRenderTarget, ID2D1SolidColorBrush* brush) {
        if (m_pShape) m_pShape->Draw(pRenderTarget, brush);
    }

    void UIButton::SetPosition(float x, float y) {
        if (m_pShape) m_pShape->SetPosition(x, y);
    }

    void UIButton::SetSize(float width, float height) {
        if (m_pShape) m_pShape->SetSize(width, height);
    }

    void UIButton::SetOnClick(std::function<void()> handler) {
        m_hOnClick = handler;
    }

    bool UIButton::OnInput(UINT msg, WPARAM wParam, LPARAM lParam) {
        if (msg == WM_LBUTTONDOWN) {
            if (m_pShape) {
                auto x = float(LOWORD(lParam));
                auto y = float(HIWORD(lParam));
                
                if (m_pShape->HitTest(x, y)) {
                    if (m_hOnClick) m_hOnClick();
                    
                    return true;
                }
            }
        }
        return false;
    }

}
