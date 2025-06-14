#include "MyUIApp.h"

void TickShape::Draw(ID2D1RenderTarget* pRenderTarget, ID2D1SolidColorBrush* brush) { }

void TickShape::SetPosition(float x, float y) { 
	m_fX = x; 
	m_fY = y;
}

void TickShape::SetSize(float width, float height) { 
	m_fW = width;
	m_fH = height; 
}

bool TickShape::HitTest(float, float) const {
	return false;
}

void TickShape::SetColor(D2D1_COLOR_F color) {
	m_xColor = color;
}

void TickShape::DrawInRect(ID2D1RenderTarget* pRenderTarget, ID2D1SolidColorBrush* brush, float x, float y, float w, float h) {
    brush->SetColor(m_xColor);
    D2D1_POINT_2F pts[3] = {
        {x + w * 0.2f, y + h * 0.5f},
        {x + w * 0.45f, y + h * 0.8f},
        {x + w * 0.8f, y + h * 0.2f}
    };
    pRenderTarget->DrawLine(pts[0], pts[1], brush, 3.0f);
    pRenderTarget->DrawLine(pts[1], pts[2], brush, 3.0f);
}

D2D1_RECT_F TickShape::GetBoundingRect() const {
    return D2D1::RectF(m_fX, m_fY, m_fX + m_fW, m_fY + m_fH);
}

void MyUIApp::UserAfterD2DInit() {
    //rectange button
    auto rectShape = std::make_shared<FD2DW::RectShape>(50, 50, 120, 40, D2D1::ColorF(D2D1::ColorF::LightGray));
    auto button = std::make_shared<FD2DW::UIButton>(rectShape);
    button->SetOnClick([]() {
        MessageBoxA(nullptr, "Button Clicked!", "Info", MB_OK);
        });
    AddUIElement(button);

    // Rectangle checkbox with custom check element (circle)
    auto checkShape = std::make_shared<FD2DW::RectShape>(250, 70, 20, 20, D2D1::ColorF(D2D1::ColorF::LightGray));
    auto checkbox = std::make_shared<FD2DW::UICheckBox>(checkShape);
    auto checkMark = std::make_shared<FD2DW::CircleShape>(0, 0, 8, D2D1::ColorF(D2D1::ColorF::Black));
    checkbox->SetCheckElement(checkMark);
    checkbox->SetCheckOffset(6, 6);
    checkbox->SetOnToggle([](bool checked) {
        MessageBoxA(nullptr, checked ? "Checked!" : "Unchecked!", "CheckBox", MB_OK);
        });
    AddUIElement(checkbox);

    // Rectangle checkbox with custom tick mark
    auto checkShape2 = std::make_shared<FD2DW::RectShape>(300, 70, 20, 20, D2D1::ColorF(D2D1::ColorF::LightGray));
    auto checkbox2 = std::make_shared<FD2DW::UICheckBox>(checkShape2);
    auto tickMark = std::make_shared<TickShape>();
    tickMark->SetColor(D2D1::ColorF(D2D1::ColorF::Green));
    checkbox2->SetCheckElement(tickMark);
    checkbox2->SetCheckOffset(2, 2);
    checkbox2->SetOnToggle([](bool checked) {
        MessageBoxA(nullptr, checked ? "Ticked!" : "Unticked!", "CheckBox", MB_OK);
        });
    AddUIElement(checkbox2);

    // ComboBox with shapes (dropdown rendering)
    auto comboShape = std::make_shared<FD2DW::RectShape>(50, 120, 120, 30, D2D1::ColorF(D2D1::ColorF::LightGray));
    auto combo = std::make_shared<FD2DW::UIComboBox>(comboShape);
    combo->AddItem(std::make_shared<FD2DW::RectShape>(0, 0, 20, 20, D2D1::ColorF(D2D1::ColorF::Red)));
    combo->AddItem(std::make_shared<FD2DW::CircleShape>(10, 10, 10, D2D1::ColorF(D2D1::ColorF::Green)));
    combo->AddItem(std::make_shared<FD2DW::TriangleShape>(D2D1::Point2F(0, 20), D2D1::Point2F(10, 0), D2D1::Point2F(20, 20), D2D1::ColorF(D2D1::ColorF::Blue)));
    combo->SetOnSelect([](size_t idx) {
        char buf[64];
        sprintf_s(buf, "ComboBox selected: %zu", idx);
        MessageBoxA(nullptr, buf, "ComboBox", MB_OK);
        });
    combo->SetDropOffset(0, 0);
    AddUIElement(combo);

    // Simple UI element (for rendering only)
    auto simpleShape = std::make_shared<FD2DW::RectShape>(250, 120, 60, 60, D2D1::ColorF(D2D1::ColorF::Yellow));
    auto simple = std::make_shared<FD2DW::UIElementSimple>(simpleShape);
    AddUIElement(simple);

    // Horizontal slider (left to right, updates on change)
    auto sliderBg = std::make_shared<FD2DW::RectShape>(50, 200, 200, 20, D2D1::ColorF(D2D1::ColorF::Gray));
    auto sliderHandle = std::make_shared<FD2DW::RectShape>(0, 0, 20, 20, D2D1::ColorF(D2D1::ColorF::Orange));
    auto slider = std::make_shared<FD2DW::UISlider>(sliderBg, sliderHandle, FD2DW::SliderOrientation::Horizontal, FD2DW::SliderDirection::LeftToRight);
    slider->SetRange(0, 100);
    slider->SetValue(50);
    slider->SetUpdateMode(FD2DW::SliderUpdateMode::OnChange);
    slider->SetOnValueChanged([](double v) {
        char buf[64];
        sprintf_s(buf, "Slider value: %.2f", v);
        OutputDebugStringA(buf);
        OutputDebugStringA("\n");
        });
    AddUIElement(slider);

    // Vertical slider (bottom to top, updates on release)
    auto sliderBgV = std::make_shared<FD2DW::RectShape>(300, 200, 20, 200, D2D1::ColorF(D2D1::ColorF::Gray));
    auto sliderHandleV = std::make_shared<FD2DW::CircleShape>(0, 0, 10, D2D1::ColorF(D2D1::ColorF::Purple));
    auto sliderV = std::make_shared<FD2DW::UISlider>(sliderBgV, sliderHandleV, FD2DW::SliderOrientation::Vertical, FD2DW::SliderDirection::BottomToTop);
    sliderV->SetRange(-1, 1);
    sliderV->SetValue(0);
    sliderV->SetUpdateMode(FD2DW::SliderUpdateMode::OnRelease);
    sliderV->SetOnValueChanged([](double v) {
        char buf[64];
        sprintf_s(buf, "Vertical slider value: %.2f", v);
        MessageBoxA(nullptr, buf, "Vertical Slider", MB_OK);
        });
    AddUIElement(sliderV);
}