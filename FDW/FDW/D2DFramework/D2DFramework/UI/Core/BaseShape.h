#pragma once
#include "pch.h"

namespace FD2DW
{
    class BaseShape {
    public:

        BaseShape()=default;
        virtual ~BaseShape()=default;

        virtual void Draw(ID2D1RenderTarget* pRenderTarget, ID2D1SolidColorBrush* brush) = 0;
        virtual void DrawInRect(ID2D1RenderTarget* pRenderTarget, ID2D1SolidColorBrush* brush, float x, float y, float w, float h);

        virtual void SetPosition(float x, float y) = 0;
        virtual void SetSize(float width, float height) = 0;
        virtual void SetColor(D2D1_COLOR_F color) = 0;

        virtual bool HitTest(float x, float y) const = 0;
        virtual D2D1_RECT_F GetBoundingRect() const = 0;
    };
}