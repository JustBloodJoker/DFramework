#include "UI/Core/UIElementBase.h"


namespace FD2DW {

	void UIElementBase::SetShape(std::shared_ptr<BaseShape> shape) { m_pShape = shape; }
	std::shared_ptr<BaseShape> UIElementBase::GetShape() const { return m_pShape; }


}
