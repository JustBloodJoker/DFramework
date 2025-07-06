#pragma once
#include "../pch.h"

namespace FDWWIN
{

	class WinWindowInputRouter;

	class WinWindowInputLayer {
	public:
		WinWindowInputLayer() = default;
		virtual ~WinWindowInputLayer() = default;

	public:
		virtual bool ProcessInput(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) = 0;

	public:
		void AddToRouter(WinWindowInputRouter* newRouter);

	private:
		WinWindowInputRouter* m_pCurrentRouter = nullptr;

	};

}