#pragma once
#include "pch.h"
#include "UI/Core/UIElementBase.h"

namespace FD2DW {

    class UIElementSimple : public UIElementBase {
    public:
        UIElementSimple(std::shared_ptr<BaseShape> shape);

        void Draw(ID2D1RenderTarget* pRenderTarget, ID2D1SolidColorBrush* brush) override;
        
        void SetPosition(float x, float y) override;
        void SetSize(float width, float height) override;
        void SetColor(D2D1_COLOR_F color);
    };

}