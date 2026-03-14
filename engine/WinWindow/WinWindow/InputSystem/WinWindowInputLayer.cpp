#include "WinWindowInputLayer.h"
#include "WinWindowInputRouter.h"

namespace FDWWIN {
	WinWindowInputLayer::~WinWindowInputLayer()
	{
		AddToRouter(nullptr);
	}

	void WinWindowInputLayer::AddToRouter(WinWindowInputRouter* newRouter) {
		if (m_pCurrentRouter == newRouter) return;
		
		if (m_pCurrentRouter) m_pCurrentRouter->RemoveLayer(this);

		m_pCurrentRouter = newRouter;

		if (m_pCurrentRouter) m_pCurrentRouter->AddLayer(this);
	}

	float WinWindowInputLayer::GetDT()
	{
		return 0.0f;
	}

	bool WinWindowInputLayer::ProcessInput(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) { return false; }
	
	void WinWindowInputLayer::PreTickUpdate(float DT) {}

	bool WinWindowInputLayer::IsKeyDown(uint8_t btn) {
		return m_pCurrentRouter->IsKeyDown(btn);
	}
}
