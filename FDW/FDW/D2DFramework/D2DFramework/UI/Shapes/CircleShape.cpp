#include "CircleShape.h"

namespace FD2DW {

    CircleShape::CircleShape(float cx, float cy, float radius, D2D1_COLOR_F color) : m_fCx(cx), m_fCy(cy), m_fRadius(radius), m_xColor(color) {}

    void CircleShape::Draw(ID2D1RenderTarget* pRenderTarget, ID2D1SolidColorBrush* brush) {
        if (!pRenderTarget || !brush) return;
        brush->SetColor(m_xColor);
        pRenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(m_fCx, m_fCy), m_fRadius, m_fRadius), brush);
    }

    void CircleShape::SetPosition(float x, float y) {
        // Need to add the radius because this method sets the position at the top-left corner.
        m_fCx = x + m_fRadius;
        m_fCy = y + m_fRadius;
    }

    void CircleShape::SetSize(float width, float height) {
        m_fRadius = (width < height ? width : height) / 2.0f;
    }

    void CircleShape::SetColor(D2D1_COLOR_F color) { 
        m_xColor = color; 
    }

    bool CircleShape::HitTest(float x, float y) const {
        float dx = x - m_fCx;
        float dy = y - m_fCy;
        return dx * dx + dy * dy <= m_fRadius * m_fRadius;
    }

    D2D1_RECT_F CircleShape::GetBoundingRect() const {
        return D2D1::RectF(m_fCx - m_fRadius, m_fCy - m_fRadius, m_fCx + m_fRadius, m_fCy + m_fRadius);
    }
}
