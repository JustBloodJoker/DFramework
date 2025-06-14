#pragma once
#include "pch.h"
#include "UI/Core/UIElementBaseWithInput.h"

namespace FD2DW {

    enum class SliderOrientation {
        Horizontal,
        Vertical
    };

    enum class SliderDirection {
        LeftToRight,
        RightToLeft,
        TopToBottom,
        BottomToTop
    };

    enum class SliderUpdateMode {
        OnChange,
        OnRelease
    };

    class UISlider : public UIElementBaseWithInput {
    public:
        UISlider(std::shared_ptr<BaseShape> background, std::shared_ptr<BaseShape> slider, SliderOrientation orientation = SliderOrientation::Horizontal, SliderDirection direction = SliderDirection::LeftToRight);

        void Draw(ID2D1RenderTarget* pRenderTarget, ID2D1SolidColorBrush* brush) override;
    
        void SetPosition(float x, float y) override;
        void SetSize(float width, float height) override;
    
        void SetRange(double min, double max);
        void SetValue(double value);
        double GetValue() const;
    
        void SetUpdateMode(SliderUpdateMode mode);
        void SetOrientation(SliderOrientation orientation);
        void SetDirection(SliderDirection direction);

        void SetOnValueChanged(std::function<void(double)> cb);
        
        bool OnInput(UINT msg, WPARAM wParam, LPARAM lParam) override;

    protected:
        void UpdateSliderPosition();
        double PositionToValue(float pos) const;
        float ValueToPosition(double value) const;

    protected:
        std::shared_ptr<BaseShape> m_pBackground;
        std::shared_ptr<BaseShape> m_pSlider;
    
        SliderOrientation m_xOrientation;
        SliderDirection m_xDirection;
        SliderUpdateMode m_xUpdateMode = SliderUpdateMode::OnChange;
        
        double m_dMin = 0.0;
        double m_dMax = 1.0;
        double m_dValue = 0.0;
        
        bool m_bDragging = false;
        
        std::function<void(double)> m_hOnValueChanged;

    };

}