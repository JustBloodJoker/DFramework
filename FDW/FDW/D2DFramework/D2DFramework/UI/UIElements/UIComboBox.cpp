#include "UI/UIElements/UIComboBox.h"

namespace FD2DW {

    UIComboBox::UIComboBox(std::shared_ptr<BaseShape> shape) {
        SetShape(shape);
    }

    void UIComboBox::Draw(ID2D1RenderTarget* pRenderTarget, ID2D1SolidColorBrush* brush) {
        if (m_pShape) m_pShape->Draw(pRenderTarget, brush);
        
        if (!m_vItems.empty() && m_vItems[m_uSelectedIndex]) {
            auto rect = m_pShape->GetBoundingRect();
            D2D1_RECT_F origRect = m_vItems[m_uSelectedIndex]->GetBoundingRect();
            
            m_vItems[m_uSelectedIndex]->SetPosition(rect.left + 5, rect.top + 5);
            m_vItems[m_uSelectedIndex]->SetSize(rect.right - rect.left - 10, rect.bottom - rect.top - 10);
            m_vItems[m_uSelectedIndex]->Draw(pRenderTarget, brush);
            m_vItems[m_uSelectedIndex]->SetPosition(origRect.left, origRect.top);
            m_vItems[m_uSelectedIndex]->SetSize(origRect.right - origRect.left, origRect.bottom - origRect.top);
        }
        
        if (m_bIsOpen) {
            auto rect = m_pShape->GetBoundingRect();
            auto x = rect.left + m_fDropOffsetX;
            auto y = rect.bottom + m_fDropOffsetY;
            auto w = rect.right - rect.left;
            auto h = rect.bottom - rect.top;

            for (auto i = 0; i < m_vItems.size(); ++i) {
                if (!m_vItems[i]) continue; 
                
                D2D1_RECT_F origRect = m_vItems[i]->GetBoundingRect();
                m_vItems[i]->SetPosition(x + 5, y + 5 + i * h);
                m_vItems[i]->SetSize(w - 10, h - 10);
                m_vItems[i]->Draw(pRenderTarget, brush);
                m_vItems[i]->SetPosition(origRect.left, origRect.top);
                m_vItems[i]->SetSize(origRect.right - origRect.left, origRect.bottom - origRect.top);
            }
        }
    }

    void UIComboBox::SetPosition(float x, float y) {
        if (m_pShape) m_pShape->SetPosition(x, y);
    }

    void UIComboBox::SetSize(float width, float height) {
        if (m_pShape) m_pShape->SetSize(width, height);
    }

    void UIComboBox::AddItem(std::shared_ptr<BaseShape> itemShape) {
        m_vItems.push_back(itemShape);
    }

    void UIComboBox::SetSelectedIndex(size_t index) {
        if (index < m_vItems.size()) m_uSelectedIndex = index;
    }

    size_t UIComboBox::GetSelectedIndex() const {
        return m_uSelectedIndex;
    }

    void UIComboBox::SetOnSelect(std::function<void(size_t)> handler) {
        m_hOnSelect = handler;
    }

    bool UIComboBox::OnInput(UINT msg, WPARAM wParam, LPARAM lParam) {
        if (msg == WM_LBUTTONDOWN) {
            auto x = float(LOWORD(lParam));
            auto y = float(HIWORD(lParam));
            if (m_pShape && m_pShape->HitTest(x, y)) {
                m_bIsOpen = !m_bIsOpen;
                return true;
            }
            if (m_bIsOpen) {
                auto rect = m_pShape->GetBoundingRect();
                auto bx = rect.left + m_fDropOffsetX;
                auto by = rect.bottom + m_fDropOffsetY;
                auto bw = rect.right - rect.left;
                auto bh = rect.bottom - rect.top;
                
                for (auto i = 0; i < m_vItems.size(); ++i) {
                    if (x >= bx && x <= bx + bw && y >= by + i * bh && y <= by + (i + 1) * bh) {
                        m_uSelectedIndex = i;
                        m_bIsOpen = false;
                        if (m_hOnSelect) m_hOnSelect(m_uSelectedIndex);
                        return true;
                    }
                }
                m_bIsOpen = false;
            }
        }
        return false;
    }

    void UIComboBox::SetDropOffset(float x, float y) {
        m_fDropOffsetX = x;
        m_fDropOffsetY = y;
    }

    bool UIComboBox::IsOpen() const { 
        return m_bIsOpen;
    }

    void UIComboBox::Close() {
        m_bIsOpen = false;
    }

}
