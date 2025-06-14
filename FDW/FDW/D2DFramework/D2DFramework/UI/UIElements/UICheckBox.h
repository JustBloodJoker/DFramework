#pragma once
#include "pch.h"
#include "UI/Core/UIElementBaseWithInput.h"

namespace FD2DW {

    class UICheckBox : public UIElementBaseWithInput {
    
    public:
        UICheckBox(std::shared_ptr<BaseShape> shape);
        virtual ~UICheckBox() = default;

        virtual void Draw(ID2D1RenderTarget* pRenderTarget, ID2D1SolidColorBrush* brush) override;
    
        virtual void SetPosition(float x, float y) override;
        virtual void SetSize(float width, float height) override;
        
        void SetChecked(bool checked);
        bool IsChecked() const;
        
        void SetOnToggle(std::function<void(bool)> handler);
        
        virtual bool OnInput(UINT msg, WPARAM wParam, LPARAM lParam) override;
        
        void SetCheckElement(std::shared_ptr<BaseShape> checkElem);
        void SetCheckOffset(float x, float y);
    
    protected:
        bool m_bChecked = false;
    
        std::function<void(bool)> m_hOnToggle;
    
        std::shared_ptr<BaseShape> m_pCheckElement;
    
        float m_fCheckOffsetX = 0.0f;
        float m_fCheckOffsetY = 0.0f;
    };

}
