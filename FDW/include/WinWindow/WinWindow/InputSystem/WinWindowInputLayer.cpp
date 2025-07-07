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

}
