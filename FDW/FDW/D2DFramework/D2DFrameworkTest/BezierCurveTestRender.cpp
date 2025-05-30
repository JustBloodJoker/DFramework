#include "BezierCurveTestRender.h"

BezierCurveTestRender::BezierCurveTestRender()
    : FDWWIN::WinWindow(L"Lab2_ComputerGraphicsSamoilenko", 800, 600, false)
{
}

void BezierCurveTestRender::UserAfterD2DInit()
{
    rtv = GetRenderTarget();
    brush = GetBrush();
}

void BezierCurveTestRender::UserD2DLoop()
{
    rtv->Clear(D2D1::ColorF(D2D1::ColorF::White));
    SetMainBrushColor(D2D1::ColorF::Red);

    std::vector<D2D1_POINT_2F> curve;
    for (int i = 0; i <= 1000; i++) {
        float t = (static_cast<float>(i) / 1000);
        curve.push_back(ComputePoint(t));
    }
    for (int i = 1; i < curve.size(); ++i) {
        rtv->DrawLine(curve[i - 1], curve[i], brush);
    }
}

void BezierCurveTestRender::UserD2DClose()
{
}

void BezierCurveTestRender::ChildKeyPressed(WPARAM param)
{
    if (param == VK_UP) {
        ScaleFactor *= 1.1;
        Scale(ScaleFactor);
    }
    else if (param == VK_DOWN) {
        ScaleFactor /= 1.1;
        Scale(ScaleFactor);
    }
    else if (param == VK_F1) {
        Rotate(0.1f);
    }
    else if (param == VK_F2) {
        Rotate(-0.1f);
    }
    else if (param == VK_F3) {
        Translate(0, -10);
    }
    else if (param == VK_F4) {
        Translate(0, 10);
    }
    else if (param == VK_F5) {
        Translate(-10, 0);
    }
    else if (param == VK_F6) {
        Translate(10, 0);
    }
    else if (param == VK_F7) {
        xMirror();
    }
    else if (param == VK_F8) {
        yMirror();
    }
}

D2D1_POINT_2F BezierCurveTestRender::ComputePoint(float t) {
    int n = controlPoints.size() - 1;
    D2D1_POINT_2F result = { 0,0 };
    for (int i = 0; i <= n; i++) {
        float coeff = BinomialCoefficient(n, i) * pow(1 - t, n - i) * pow(t, i);
        result.x += coeff * controlPoints[i].x;
        result.y += coeff * controlPoints[i].y;
    }
    return result;
}

void BezierCurveTestRender::Rotate(float angle) {
    D2D1_POINT_2F center{ 0, 0 };
    for (const auto& point : controlPoints) {
        center.x += point.x;
        center.y += point.y;
    }
    center.x /= static_cast<float>(controlPoints.size());
    center.y /= static_cast<float>(controlPoints.size());

    for (auto& point : controlPoints) {
        float x = point.x - center.x;
        float y = point.y - center.y;
        float xNew = x * cos(angle) - y * sin(angle);
        float yNew = x * sin(angle) + y * cos(angle);
        point.x = xNew + center.x;
        point.y = yNew + center.y;
    }
}

void BezierCurveTestRender::Scale(float scale) {
    D2D1_POINT_2F center = { 0, 0 };
    for (const auto& point : controlPoints) {
        center.x += point.x;
        center.y += point.y;
    }
    center.x /= static_cast<float>(controlPoints.size());
    center.y /= static_cast<float>(controlPoints.size());

    for (auto& point : controlPoints) {
        point.x *= scale;
        point.y *= scale;
    }
}

void BezierCurveTestRender::Translate(float dx, float dy) {
    for (auto& point : controlPoints) {
        point.x += dx;
        point.y += dy;
    }
}

void BezierCurveTestRender::xMirror() {
    for (auto& point : controlPoints) {
        point.y = WNDSettings().Height - point.y;
    }
}

void BezierCurveTestRender::yMirror() {
    for (auto& point : controlPoints) {
        point.x = WNDSettings().Width - point.x;
    }
}

int BezierCurveTestRender::BinomialCoefficient(int n, int k) {
    int result = 1;
    for (int i = 1; i <= k; i++) {
        result *= (n - i + 1);
        result /= i;
    }
    return result;
}