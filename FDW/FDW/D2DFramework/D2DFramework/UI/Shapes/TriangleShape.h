#pragma once
#include "pch.h"
#include "UI/Core/BaseShape.h"

namespace FD2DW {

    class TriangleShape : public BaseShape {
    public:
        TriangleShape(D2D1_POINT_2F p1, D2D1_POINT_2F p2, D2D1_POINT_2F p3, D2D1_COLOR_F color);

        void Draw(ID2D1RenderTarget* pRenderTarget, ID2D1SolidColorBrush* brush) override;
       
        void SetPosition(float x, float y) override;
        void SetSize(float width, float height) override;
        void SetColor(D2D1_COLOR_F color);
        
        bool HitTest(float x, float y) const override;
        D2D1_RECT_F GetBoundingRect() const override;

    private:
        std::array<D2D1_POINT_2F, 3> m_aPoints;
        D2D1_COLOR_F m_xColor;
    };

}