#pragma once
#include "pch.h"
#include <D2DFramework/D2DFW.h>

namespace FDW {

	class D2DFWDDXGI : virtual public FD2DW::D2DFW {
	public:
		D2DFWDDXGI() = default;
		virtual ~D2DFWDDXGI() = default;

	private:
		wrl::ComPtr<IDXGIDevice> pDXGIDevice;
		wrl::ComPtr<ID2D1Device> pD2DDevice;
		wrl::ComPtr<ID2D1DeviceContext> pD2DDeviceContext;
		
		wrl::ComPtr<IDXGISurface> pSwapChainSurfaces;

	};

}


