#include "pch.h"
#include "D2DFWStandalone/D2DFWUIStandalone.h"

namespace FD2DW {


	void D2DFWUIStandalone::ChildMOUSEUP(WPARAM btnState, int x, int y) {
		ProcessInput(WM_LBUTTONUP, btnState, MAKELPARAM(x, y));
	}

	void D2DFWUIStandalone::ChildMOUSEDOWN(WPARAM btnState, int x, int y) {
		ProcessInput(WM_LBUTTONDOWN, btnState, MAKELPARAM(x, y));
	}

	void D2DFWUIStandalone::ChildMOUSEMOVE(WPARAM btnState, int x, int y) {
		ProcessInput(WM_MOUSEMOVE, btnState, MAKELPARAM(x, y));
	}

	void D2DFWUIStandalone::ChildKeyPressed(WPARAM key) {
		ProcessInput(WM_KEYDOWN, key, 0);
	}

	ID2D1RenderTarget* D2DFWUIStandalone::GetRenderTarget() const {
		return D2DFWStandalone::GetRenderTarget();
	}

	ID2D1SolidColorBrush* D2DFWUIStandalone::GetBrush() const {
		return D2DFWStandalone::GetBrush();
	}

	void D2DFWUIStandalone::UserD2DClose() { }

	void D2DFWUIStandalone::UserAfterD2DInit() {
		UserUICreateFunction();
	}

	void D2DFWUIStandalone::UserD2DLoop() {
		D2DUILoop();
	}



}

