#pragma once
#include "pch.h"
#include "UI/Core/BaseShape.h"

namespace FD2DW {

    class RectShape : public BaseShape {
    public:
        RectShape(float x, float y, float width, float height, D2D1_COLOR_F color);

        void Draw(ID2D1RenderTarget* pRenderTarget, ID2D1SolidColorBrush* brush) override;
        
        void SetPosition(float x, float y) override;
        void SetSize(float width, float height) override;
        void SetColor(D2D1_COLOR_F color);
        
        bool HitTest(float x, float y) const override;
        D2D1_RECT_F GetBoundingRect() const override;

    private:
        float m_fX = 0.0f;
        float m_fY = 0.0f;
        float m_fWidth = 0.0f;
        float m_fHeight = 0.0f;

        D2D1_COLOR_F m_xColor;
    };

}
