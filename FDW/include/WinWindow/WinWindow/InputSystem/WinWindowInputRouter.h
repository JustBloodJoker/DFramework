#pragma once
#include "../pch.h"
#include "WinWindowInputLayer.h"

namespace FDWWIN
{

	class WinWindowInputRouter {
	public:
		WinWindowInputRouter() = default;
		virtual ~WinWindowInputRouter() = default;

	public:
		void AddLayer(WinWindowInputLayer* layer);
		void RemoveLayer(WinWindowInputLayer* layer);
		bool RouteInput(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	private:
		std::vector<WinWindowInputLayer*> m_vInputLayers;

	};

}