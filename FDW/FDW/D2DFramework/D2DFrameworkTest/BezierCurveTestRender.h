#pragma once
#define _DISABLE_FDW_MACROSES
#include <D2DFWStandalone.h>

class BezierCurveTestRender :
    public FD2DW::D2DFWStandalone
{
public:
    BezierCurveTestRender();
    virtual ~BezierCurveTestRender() = default;

public:
    virtual void UserAfterD2DInit() override;
    virtual void UserD2DLoop() override;
    virtual void UserD2DClose() override;
    virtual void ChildKeyPressed(WPARAM) override;
public:

protected:
    ID2D1HwndRenderTarget* rtv;
    ID2D1SolidColorBrush* brush;

    std::vector<D2D1_POINT_2F> controlPoints = {
        {100, 200},
        {1000, 300},
        {105, 247},
        {159, 533},
        {486, -185},
        {415, 352}
    };

    float ScaleFactor = 1.0f;

    ////////////
    // LAB 2 METHODS
    D2D1_POINT_2F ComputePoint(float t);
    void Rotate(float angle);
    void Scale(float scale);
    void Translate(float dx, float dy);
    void xMirror();
    void yMirror();

private:
    int BinomialCoefficient(int n, int k);
};