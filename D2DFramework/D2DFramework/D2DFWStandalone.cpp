#include "D2DFWStandalone.h"

namespace FD2DW {

	ID2D1Factory* D2DFWStandalone::GetFactory() const noexcept
	{
		return pFactory.Get();
	}

	ID2D1HwndRenderTarget* D2DFWStandalone::GetRenderTarget() const noexcept
	{
		return pRenderTarget.Get();
	}

	ID2D1SolidColorBrush* D2DFWStandalone::GetBrush() const noexcept
	{
		return pBrush.Get();
	}

	RECT D2DFWStandalone::GetMainRect() const noexcept
	{
		return mainRect;
	}

	void D2DFWStandalone::SetMainBrushColor(D2D1::ColorF color)
	{
		SetBrushColor(pBrush.Get(), color);
	}

	void D2DFWStandalone::SetBrushColor(ID2D1SolidColorBrush* brush, D2D1::ColorF color)
	{
		brush->SetColor(D2D1::ColorF(color));
	}

	void D2DFWStandalone::ChildEXITSIZE()
	{
		GetClientRect(GETHWND(), &mainRect);
		pRenderTarget->Resize(D2D1::SizeU(mainRect.right, mainRect.bottom));
	}

	void D2DFWStandalone::ChildAfterUserLoop()
	{
		EndDrawMainRenderTarget();
	}

	void D2DFWStandalone::ChildBeforeUserLoop()
	{
		pRenderTarget->BeginDraw();
	}

	bool D2DFWStandalone::InitD2D()
	{
		if ( FAILED(hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, pFactory.GetAddressOf())) )
		{
			HRESULT_ASSERT(hr, "D2D Factory create error");
			return false;
		}
		
		const auto& hwnd = GETHWND();
		GetClientRect(hwnd, &mainRect);
		if ( FAILED(hr = pFactory->CreateHwndRenderTarget(
			D2D1::RenderTargetProperties(),
			D2D1::HwndRenderTargetProperties(hwnd, D2D1::SizeU(mainRect.right, mainRect.bottom)),
			&pRenderTarget)) ) 
		{
			HRESULT_ASSERT(hr, "D2D RTV create error");
			return false;
		}

		if ( FAILED(hr = pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &pBrush)) )
		{
			HRESULT_ASSERT(hr, "D2D Brush create error");
			return false;
		}
		
		return true;
	}

	void D2DFWStandalone::EndDrawMainRenderTarget()
	{
		HRESULT_ASSERT(pRenderTarget->EndDraw(), "MAIN RENDER TARGET SWAP ERROR");
	}
}

