#include "UI/Shapes/RectShape.h"

namespace FD2DW {

	RectShape::RectShape(float x, float y, float width, float height, D2D1_COLOR_F color) : m_fX(x), m_fY(y), m_fWidth(width), m_fHeight(height), m_xColor(color) {}

	void RectShape::Draw(ID2D1RenderTarget* pRenderTarget, ID2D1SolidColorBrush* brush) {
        if (!pRenderTarget || !brush) return;
        brush->SetColor(m_xColor);
        pRenderTarget->FillRectangle(D2D1::RectF(m_fX, m_fY, m_fX + m_fWidth, m_fY + m_fHeight), brush);
    }

    void RectShape::SetPosition(float x, float y) {
        m_fX = x; 
        m_fY = y;
    }

    void RectShape::SetSize(float width, float height) {
        m_fWidth = width; 
        m_fHeight = height;
    }

    void RectShape::SetColor(D2D1_COLOR_F color) { 
        m_xColor = color;
    }

    bool RectShape::HitTest(float x, float y) const {
        return x >= m_fX && x <= m_fX + m_fWidth && y >= m_fY && y <= m_fY + m_fHeight;
    }

    D2D1_RECT_F RectShape::GetBoundingRect() const {
        return D2D1::RectF(m_fX, m_fY, m_fX + m_fWidth, m_fY + m_fHeight);
    }

}