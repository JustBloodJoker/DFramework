#include "UI/UIElements/UISlider.h"

namespace FD2DW {

UISlider::UISlider(std::shared_ptr<BaseShape> background, std::shared_ptr<BaseShape> slider, SliderOrientation orientation, SliderDirection direction)
    : m_pBackground(background), m_pSlider(slider), m_xOrientation(orientation), m_xDirection(direction) {
    UpdateSliderPosition();
}

void UISlider::Draw(ID2D1RenderTarget* pRenderTarget, ID2D1SolidColorBrush* brush) {
    if (m_pBackground) m_pBackground->Draw(pRenderTarget, brush);
    if (m_pSlider) m_pSlider->Draw(pRenderTarget, brush);
}

void UISlider::SetPosition(float x, float y) {
    if (m_pBackground) m_pBackground->SetPosition(x, y);
    UpdateSliderPosition();
}

void UISlider::SetSize(float width, float height) {
    if (m_pBackground) m_pBackground->SetSize(width, height);
    UpdateSliderPosition();
}

void UISlider::SetRange(double min, double max) {
    m_dMin = min;
    m_dMax = max;
    UpdateSliderPosition();
}

void UISlider::SetValue(double value) {
    m_dValue = std::max(m_dMin, std::min(m_dMax, value));
    UpdateSliderPosition();
    if (m_xUpdateMode == SliderUpdateMode::OnChange && m_hOnValueChanged) m_hOnValueChanged(m_dValue);
}

double UISlider::GetValue() const {
    return m_dValue;
}

void UISlider::SetUpdateMode(SliderUpdateMode mode) {
    m_xUpdateMode = mode;
}

void UISlider::SetOnValueChanged(std::function<void(double)> cb) {
    m_hOnValueChanged = cb;
}

void UISlider::SetOrientation(SliderOrientation orientation) {
    m_xOrientation = orientation;
    UpdateSliderPosition();
}

void UISlider::SetDirection(SliderDirection direction) {
    m_xDirection = direction;
    UpdateSliderPosition();
}

void UISlider::UpdateSliderPosition() {
    if (!m_pBackground || !m_pSlider) return;
    
    auto rect = m_pBackground->GetBoundingRect();
    auto x = rect.left;
    auto y = rect.top;
    auto w = rect.right - rect.left;
    auto h = rect.bottom - rect.top;
    auto t = float((m_dValue - m_dMin) / (m_dMax - m_dMin));
    
    if (m_xOrientation == SliderOrientation::Horizontal) {
        auto sliderW = w * 0.1f;
        auto sliderH = h;
        auto pos = 0.0f;

        if (m_xDirection == SliderDirection::LeftToRight) {
            pos = x + t * (w - sliderW);
        } else {
            pos = x + (1.0f - t) * (w - sliderW);
        }

        m_pSlider->SetPosition(pos, y);
        m_pSlider->SetSize(sliderW, sliderH);
    
    } else {
    
        auto sliderW = w;
        auto sliderH = h * 0.1f;
        auto pos = 0.0f;
        
        if (m_xDirection == SliderDirection::TopToBottom) {
            pos = y + t * (h - sliderH);
        } else {
            pos = y + (1.0f - t) * (h - sliderH);
        }
    
        m_pSlider->SetPosition(x, pos);
        m_pSlider->SetSize(sliderW, sliderH);
    }
}

double UISlider::PositionToValue(float pos) const {
    if (!m_pBackground) return m_dMin;
    
    auto rect = m_pBackground->GetBoundingRect();
    auto x = rect.left;
    auto y = rect.top;
    auto w = rect.right - rect.left;
    auto h = rect.bottom - rect.top;
    auto t = 0.0f;

    if (m_xOrientation == SliderOrientation::Horizontal) {
        auto sliderW = w * 0.1f;
        if (m_xDirection == SliderDirection::LeftToRight)
        {
            t = (pos - x) / (w - sliderW);
        } else {
            t = 1.0f - (pos - x) / (w - sliderW);
        }
    } else {
        auto sliderH = h * 0.1f;
        if (m_xDirection == SliderDirection::TopToBottom) {
            t = (pos - y) / (h - sliderH);
        } else{
            t = 1.0f - (pos - y) / (h - sliderH);
        }
    }
    t = std::max(0.0f, std::min(1.0f, t));
    return m_dMin + t * (m_dMax - m_dMin);
}

float UISlider::ValueToPosition(double value) const {
    if (!m_pBackground) return 0.0f;
    
    auto rect = m_pBackground->GetBoundingRect();
    auto x = rect.left;
    auto y = rect.top;
    auto w = rect.right - rect.left;
    auto h = rect.bottom - rect.top;
    auto t = float((value - m_dMin) / (m_dMax - m_dMin));

    if (m_xOrientation == SliderOrientation::Horizontal) {
        auto sliderW = w * 0.1f;
        if (m_xDirection == SliderDirection::LeftToRight) {
            return x + t * (w - sliderW);
        } else {
            return x + (1.0f - t) * (w - sliderW);
        }
    } else {
        auto sliderH = h * 0.1f;
        if (m_xDirection == SliderDirection::TopToBottom) {
            return y + t * (h - sliderH);
        } else{
            return y + (1.0f - t) * (h - sliderH);
        }
    }
}

bool UISlider::OnInput(UINT msg, WPARAM wParam, LPARAM lParam) {
    auto mx = float(LOWORD(lParam));
    auto my = float(HIWORD(lParam));
    if (msg == WM_LBUTTONDOWN) {
        if (m_pSlider && m_pSlider->HitTest(mx, my)) {
            m_bDragging = true;
            return true;
        }
    } else if (msg == WM_MOUSEMOVE && m_bDragging) {
        auto rect = m_pBackground->GetBoundingRect();
        if (m_xOrientation == SliderOrientation::Horizontal) {
            SetValue(PositionToValue(mx));
        } else {
            SetValue(PositionToValue(my));
        }
        return true;
    } else if (msg == WM_LBUTTONUP && m_bDragging) {
        m_bDragging = false;
        
        if (m_xUpdateMode == SliderUpdateMode::OnRelease && m_hOnValueChanged) m_hOnValueChanged(m_dValue);
        
        return true;
    }
    return false;
}

}
