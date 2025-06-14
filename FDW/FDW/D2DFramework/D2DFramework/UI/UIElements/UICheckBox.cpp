#include "UICheckBox.h"

namespace FD2DW {
    UICheckBox::UICheckBox(std::shared_ptr<BaseShape> shape) {
        SetShape(shape);
    }

    void UICheckBox::Draw(ID2D1RenderTarget* pRenderTarget, ID2D1SolidColorBrush* brush) {
        if (m_pShape) m_pShape->Draw(pRenderTarget, brush);

        if (m_bChecked && m_pCheckElement) {
            auto rect = m_pShape->GetBoundingRect();
            auto baseX = rect.left;
            auto baseY = rect.top;
            auto w = rect.right - rect.left;
            auto h = rect.bottom - rect.top;
            m_pCheckElement->DrawInRect(pRenderTarget, brush, baseX+m_fCheckOffsetX, baseY+m_fCheckOffsetY, w-2*m_fCheckOffsetX, h-2*m_fCheckOffsetY);
        }
    }

    void UICheckBox::SetPosition(float x, float y) {
        if (m_pShape) m_pShape->SetPosition(x, y);
    }

    void UICheckBox::SetSize(float width, float height) {
        if (m_pShape) m_pShape->SetSize(width, height);
    }

    void UICheckBox::SetChecked(bool checked) {
        m_bChecked = checked;
    }

    bool UICheckBox::IsChecked() const {
        return m_bChecked;
    }

    void UICheckBox::SetOnToggle(std::function<void(bool)> handler) { m_hOnToggle = handler; }

    bool UICheckBox::OnInput(UINT msg, WPARAM wParam, LPARAM lParam) {
        if (msg == WM_LBUTTONDOWN) {
            if (m_pShape) {
                auto x = float(LOWORD(lParam));
                auto y = float(HIWORD(lParam));
                if (m_pShape->HitTest(x, y)) {
                    m_bChecked = !m_bChecked;
                    
                    if (m_hOnToggle) m_hOnToggle(m_bChecked);
                    
                    return true;
                }
            }
        }
        return false;
    }

    void UICheckBox::SetCheckElement(std::shared_ptr<BaseShape> checkElem) { 
        m_pCheckElement = checkElem; 
    }

    void UICheckBox::SetCheckOffset(float x, float y) { 
        m_fCheckOffsetX = x;
        m_fCheckOffsetY = y;
    }

}