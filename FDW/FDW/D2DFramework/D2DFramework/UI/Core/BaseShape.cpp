#include "UI/Core/BaseShape.h"

namespace FD2DW {
	
    void BaseShape::DrawInRect(ID2D1RenderTarget* pRenderTarget, ID2D1SolidColorBrush* brush, float x, float y, float w, float h) {
        SetPosition(x, y);
        SetSize(w, h);
        Draw(pRenderTarget, brush);
	}

}
