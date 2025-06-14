#pragma once
#include "pch.h"
#include "UI/Core/UIElementBaseWithInput.h"

namespace FD2DW {

    class UIButton : public UIElementBaseWithInput {
    public:
        
        UIButton(std::shared_ptr<BaseShape> shape);
        virtual ~UIButton() = default;

        virtual void Draw(ID2D1RenderTarget* pRenderTarget, ID2D1SolidColorBrush* brush) override;
        
        virtual void SetPosition(float x, float y) override;
        virtual void SetSize(float width, float height) override;

        void SetOnClick(std::function<void()> handler);
        
        virtual bool OnInput(UINT msg, WPARAM wParam, LPARAM lParam) override;

    protected:
        std::function<void()> m_hOnClick;
    }; 

}
