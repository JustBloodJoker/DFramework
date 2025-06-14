#include "UI/UIElements/UIElementSimple.h"

namespace FD2DW {

	UIElementSimple::UIElementSimple(std::shared_ptr<BaseShape> shape) { 
		SetShape(shape); 
	}

	void UIElementSimple::Draw(ID2D1RenderTarget* pRenderTarget, ID2D1SolidColorBrush* brush) {
		if (m_pShape) m_pShape->Draw(pRenderTarget, brush);
	}
	void UIElementSimple::SetPosition(float x, float y) {
		if (m_pShape) m_pShape->SetPosition(x, y);
	}
	void UIElementSimple::SetSize(float width, float height) {
		if (m_pShape) m_pShape->SetSize(width, height);
	}
	void UIElementSimple::SetColor(D2D1_COLOR_F color) {
		if (m_pShape) m_pShape->SetColor(color);
	}
}