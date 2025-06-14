#pragma once
#include "pch.h"
#include <D2DFramework/D2DFW.h>
#include <D2DFramework/D2DFW_UIController.h>

namespace FDW {

	class D2DFW_DXGI_UIController : virtual public FD2DW::D2DFW_UIController {
	public:
		D2DFW_DXGI_UIController() = default;
		virtual ~D2DFW_DXGI_UIController() = default;

		ID2D1RenderTarget* GetRenderTarget() const override { /* TODO: return DXGI render target */ return nullptr; }
		ID2D1SolidColorBrush* GetBrush() const override { /* TODO: return DXGI brush */ return nullptr; }

	private:
		wrl::ComPtr<IDXGIDevice> m_pDXGIDevice;
		wrl::ComPtr<ID2D1Device> m_pD2DDevice;
		wrl::ComPtr<ID2D1DeviceContext> m_pD2DDeviceContext;

		wrl::ComPtr<IDXGISurface> m_pSwapChainSurfaces;

	};

}


