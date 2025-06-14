#include "D2DFWStandalone/D2DFWStandalone.h"

namespace FD2DW {

	bool D2DFWStandalone::ChildInit()
	{
		if (InitD2D()) {
			CONSOLE_MESSAGE("D2D INITED");
		}

		CONSOLE_MESSAGE("CALL USER INIT");
		UserAfterD2DInit();

		return true;
	}

	void D2DFWStandalone::ChildLoop()
	{
		ChildBeforeUserLoop();
		UserD2DLoop();
		ChildAfterUserLoop();
	}

	void D2DFWStandalone::ChildRelease()
	{
		UserD2DClose();
		CONSOLE_MESSAGE("USER CLOSED");
	}

	void D2DFWStandalone::ChildKeyPressed(WPARAM param)
	{
	}

	void D2DFWStandalone::ChildSIZE()
	{

	}

	void D2DFWStandalone::ChildENTERSIZE()
	{

	}

	void D2DFWStandalone::ChildMOUSEUP(WPARAM btnState, int x, int y)
	{
	}

	void D2DFWStandalone::ChildMOUSEMOVE(WPARAM btnState, int x, int y)
	{
	}

	void D2DFWStandalone::ChildMOUSEDOWN(WPARAM btnState, int x, int y)
	{
	}

	ID2D1Factory* D2DFWStandalone::GetFactory() const noexcept
	{
		return m_pFactory.Get();
	}

	ID2D1HwndRenderTarget* D2DFWStandalone::GetRenderTarget() const noexcept
	{
		return m_pRenderTarget.Get();
	}

	ID2D1SolidColorBrush* D2DFWStandalone::GetBrush() const noexcept
	{
		return m_pBrush.Get();
	}

	RECT D2DFWStandalone::GetMainRect() const noexcept
	{
		return m_xMainRect;
	}

	void D2DFWStandalone::SetMainBrushColor(D2D1::ColorF color)
	{
		SetBrushColor(m_pBrush.Get(), color);
	}

	void D2DFWStandalone::SetBrushColor(ID2D1SolidColorBrush* brush, D2D1::ColorF color)
	{
		brush->SetColor(D2D1::ColorF(color));
	}

	void D2DFWStandalone::ChildEXITSIZE()
	{
		GetClientRect(GETHWND(), &m_xMainRect);
		m_pRenderTarget->Resize(D2D1::SizeU(m_xMainRect.right, m_xMainRect.bottom));
	}

	void D2DFWStandalone::ChildAfterUserLoop()
	{
		EndDrawMainRenderTarget();
	}

	void D2DFWStandalone::ChildBeforeUserLoop()
	{
		m_pRenderTarget->BeginDraw();
	}

	bool D2DFWStandalone::InitD2D()
	{
		if ( FAILED(hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, m_pFactory.GetAddressOf())) )
		{
			HRESULT_ASSERT(hr, "D2D Factory create error");
			return false;
		}
		
		const auto& hwnd = GETHWND();
		GetClientRect(hwnd, &m_xMainRect);
		if ( FAILED(hr = m_pFactory->CreateHwndRenderTarget(
			D2D1::RenderTargetProperties(),
			D2D1::HwndRenderTargetProperties(hwnd, D2D1::SizeU(m_xMainRect.right, m_xMainRect.bottom)),
			&m_pRenderTarget)) )
		{
			HRESULT_ASSERT(hr, "D2D RTV create error");
			return false;
		}

		if ( FAILED(hr = m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &m_pBrush)) )
		{
			HRESULT_ASSERT(hr, "D2D Brush create error");
			return false;
		}
		
		return true;
	}

	void D2DFWStandalone::EndDrawMainRenderTarget()
	{
		HRESULT_ASSERT(m_pRenderTarget->EndDraw(), "MAIN RENDER TARGET SWAP ERROR");
	}
}

