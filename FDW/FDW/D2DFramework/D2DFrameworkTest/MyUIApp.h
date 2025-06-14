#pragma once
#include "pch.h"
#include "D2DFWUI.h"
#include "UI/Shapes/RectShape.h"
#include "UI/Shapes/CircleShape.h"
#include "UI/Shapes/TriangleShape.h"
#include "UI/UIElements/UIButton.h"
#include "UI/UIElements/UICheckBox.h"
#include "UI/UIElements/UIComboBox.h"
#include "UI/UIElements/UIElementSimple.h"
#include "UI/UIElements/UISlider.h"

class TickShape : public FD2DW::BaseShape {

public:
    void Draw(ID2D1RenderTarget* pRenderTarget, ID2D1SolidColorBrush* brush) override;
    void SetPosition(float x, float y) override;
    void SetSize(float width, float height) override;
    bool HitTest(float, float) const override;
    void SetColor(D2D1_COLOR_F color) override;
    void DrawInRect(ID2D1RenderTarget* pRenderTarget, ID2D1SolidColorBrush* brush, float x, float y, float w, float h) override;
    D2D1_RECT_F GetBoundingRect() const override;

protected:
    float m_fX = 0;
    float m_fY = 0;
    float m_fW = 0;
    float m_fH = 0;
    D2D1_COLOR_F m_xColor = D2D1::ColorF(D2D1::ColorF::Black);
};

class MyUIApp : public FD2DW::D2DFWUI {

public:
    void UserAfterD2DInit() override;

}; 