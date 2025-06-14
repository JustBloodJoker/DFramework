#pragma once
#include "pch.h"
#include "UI/Core/BaseShape.h"

namespace FD2DW {

    class UIElementBase {

    public:
        UIElementBase() = default;
        virtual ~UIElementBase() = default;

        virtual void Draw(ID2D1RenderTarget* pRenderTarget, ID2D1SolidColorBrush* brush) = 0;

        virtual void SetPosition(float x, float y) = 0;
        virtual void SetSize(float width, float height) = 0;

        void SetShape(std::shared_ptr<BaseShape> shape);
        std::shared_ptr<BaseShape> GetShape() const;

    protected:
        std::shared_ptr<BaseShape> m_pShape;
    };

}