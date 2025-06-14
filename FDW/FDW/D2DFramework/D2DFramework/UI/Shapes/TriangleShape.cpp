#include "UI/Shapes/TriangleShape.h"

namespace FD2DW {

	TriangleShape::TriangleShape(D2D1_POINT_2F p1, D2D1_POINT_2F p2, D2D1_POINT_2F p3, D2D1_COLOR_F color) : m_aPoints{ p1, p2, p3 }, m_xColor(color) {}

	void TriangleShape::Draw(ID2D1RenderTarget* pRenderTarget, ID2D1SolidColorBrush* brush) {
		if (!pRenderTarget || !brush) return;
		brush->SetColor(m_xColor);
		D2D1_POINT_2F points[3] = { m_aPoints[0], m_aPoints[1], m_aPoints[2] };

		ID2D1PathGeometry* pGeometry = nullptr;
		ID2D1Factory* pFactory = nullptr;
		pRenderTarget->GetFactory(&pFactory);
		pFactory->CreatePathGeometry(&pGeometry);

		ID2D1GeometrySink* pSink = nullptr;
		pGeometry->Open(&pSink);
		pSink->BeginFigure(points[0], D2D1_FIGURE_BEGIN_FILLED);
		pSink->AddLine(points[1]);
		pSink->AddLine(points[2]);
		pSink->EndFigure(D2D1_FIGURE_END_CLOSED);
		pSink->Close();

		pRenderTarget->FillGeometry(pGeometry, brush);

		pSink->Release();
		pGeometry->Release();
		pFactory->Release();
	}

	void TriangleShape::SetPosition(float x, float y) {
		float dx = x - m_aPoints[0].x;
		float dy = y - m_aPoints[0].y;
		for (auto& pt : m_aPoints) { pt.x += dx; pt.y += dy; }
	}

	void TriangleShape::SetSize(float width, float height) {
		// Simple resize: scale triangle to fit bounding box (not perfect)
		float minX = m_aPoints[0].x, minY = m_aPoints[0].y;
		
		for (const auto& pt : m_aPoints) {
			if (pt.x < minX) minX = pt.x;
			if (pt.y < minY) minY = pt.y;
		}
		
		m_aPoints[1].x = minX + width;
		m_aPoints[2].y = minY + height;
	}

	void TriangleShape::SetColor(D2D1_COLOR_F color) { m_xColor = color; }

	bool TriangleShape::HitTest(float x, float y) const {
		// Barycentric coordinates
		const auto& A = m_aPoints[0];
		const auto& B = m_aPoints[1];
		const auto& C = m_aPoints[2];
		auto denom = (B.y - C.y) * (A.x - C.x) + (C.x - B.x) * (A.y - C.y);
		auto a = ((B.y - C.y) * (x - C.x) + (C.x - B.x) * (y - C.y)) / denom;
		auto b = ((C.y - A.y) * (x - C.x) + (A.x - C.x) * (y - C.y)) / denom;
		auto c = 1.0f - a - b;
		return a >= 0.0f && b >= 0.0f && c >= 0.0f && a <= 1.0f && b <= 1.0f && c <= 1.0f;
	}

	D2D1_RECT_F TriangleShape::GetBoundingRect() const {
		auto minX = m_aPoints[0].x;
		auto minY = m_aPoints[0].y;
		auto maxX = m_aPoints[0].x;
		auto maxY = m_aPoints[0].y;
		for (const auto& pt : m_aPoints) {
			if (pt.x < minX) minX = pt.x;
			if (pt.y < minY) minY = pt.y;
			if (pt.x > maxX) maxX = pt.x;
			if (pt.y > maxY) maxY = pt.y;
		}
		return D2D1::RectF(minX, minY, maxX, maxY);
	}



}