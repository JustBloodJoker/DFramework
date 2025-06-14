#pragma once
#include "pch.h"
#include "UI/Core/BaseShape.h"

namespace FD2DW {

    class CircleShape : public BaseShape {
    public:
        CircleShape(float cx, float cy, float radius, D2D1_COLOR_F color);

        void Draw(ID2D1RenderTarget* pRenderTarget, ID2D1SolidColorBrush* brush) override;
        
        void SetPosition(float x, float y) override;
        void SetSize(float width, float height) override;
        void SetColor(D2D1_COLOR_F color);
        
        bool HitTest(float x, float y) const override;
        D2D1_RECT_F GetBoundingRect() const override;

    protected:
        
        float m_fCx=0.0f; 
        float m_fCy = 0.0f;
        float m_fRadius = 0.0f;

        D2D1_COLOR_F m_xColor;
    };

}
