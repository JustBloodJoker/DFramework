#pragma once
#include "../pch.h"

namespace FDWWIN
{

	class WinWindowInputRouter;

	class WinWindowInputLayer {
	public:
		WinWindowInputLayer() = default;
		virtual ~WinWindowInputLayer();

	public:
		virtual bool ProcessInput(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
		virtual void PreTickUpdate(float DT);
		bool IsKeyDown(uint8_t btn);

	public:
		void AddToRouter(WinWindowInputRouter* newRouter);

		float GetDT();

	private:
		WinWindowInputRouter* m_pCurrentRouter = nullptr;

	};

}