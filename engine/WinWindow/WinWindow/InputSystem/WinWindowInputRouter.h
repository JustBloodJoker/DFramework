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

		void PreTickUpdate(float DT);

		bool IsKeyDown(uint8_t key) const;


	private:
		std::vector<WinWindowInputLayer*> m_vInputLayers;

		bool m_aKeyStates[256] = {false};
	};

}