#include "pch.h"
#include "WinWindow.h"

namespace FDWWIN 
{
	WinWindow* WinWindow::instance = nullptr;

	WinWindow* WinWindow::GetWINInstance()
	{
		return instance;
	}

	LRESULT WinWindow::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		return WinWindow::GetWINInstance()->MsgProc(hWnd, msg, wParam, lParam);
	}
	
	WinWindow::WinWindow(std::wstring windowTittle, int width, int height, bool fullScreen)
	{
		if (!WinWindow::instance)
		{
			WinWindow::instance = this;
		}

		wndSettings.fullScreen = fullScreen;
		wndSettings.height = height;
		wndSettings.width = width;
		wndSettings.tittleName = windowTittle;

		PAUSEWORK = false;
	}
	
	void WinWindow::__START()
	{
		auto start = std::chrono::high_resolution_clock::now();

		if (InitWindow())
		{
			CONSOLE_MESSAGE("Window created");
		}
		
		if (InitTimer())
		{
			CONSOLE_MESSAGE("TIMER OBJECT INITED");
		}

		if (ChildInit())
		{
			CONSOLE_MESSAGE("Child inited");
		}

		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start);

		CONSOLE_MESSAGE(std::to_string(duration.count()) + "ms     ---------- Init Time");

		Loop();

		CONSOLE_MESSAGE("WND NOT ENABLE! END LOOP");

		Release();
	}

	void WinWindow::SETHWND(HWND& hwnd)
	{
		hwnd = this->hwnd;
	}

	void WinWindow::Release() {
		ChildRelease();

		pTimer.release();
		CONSOLE_MESSAGE("WINWINDOW RELEASE");
	}

	void WinWindow::SetFullScreen()
	{
		if (!wndSettings.fullScreen)
		{
			wndSettings.fullScreen = true;
			HMONITOR hmon = MonitorFromWindow(hwnd,
				MONITOR_DEFAULTTONEAREST);
			MONITORINFO mi = { sizeof(mi) };
			GetMonitorInfo(hmon, &mi);
			wndSettings.width = mi.rcMonitor.right - mi.rcMonitor.left;
			wndSettings.height = mi.rcMonitor.bottom - mi.rcMonitor.top;
		}
	}

	bool WinWindow::InitWindow()
	{
		CONSOLE_MESSAGE("Creating window");

		WNDCLASSEX wc = {};

		wc.cbSize = sizeof(WNDCLASSEX);
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = WinWindow::WndProc;
		wc.cbClsExtra = NULL;
		wc.cbWndExtra = NULL;
		wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 2);
		wc.lpszMenuName = NULL;
		wc.lpszClassName = L"wndClass";
		wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

		SAFE_ASSERT(RegisterClassEx(&wc), "Register wnd class error");

		hwnd = CreateWindowEx(NULL,
			L"wndClass",
			this->wndSettings.tittleName.c_str(),
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT,
			this->wndSettings.width, this->wndSettings.height,
			NULL,
			NULL,
			NULL,
			NULL);

		SAFE_ASSERT(hwnd, "Create hwnd error");

		ShowWindow(hwnd, 1);
		UpdateWindow(hwnd);
		return true;
	}

	bool WinWindow::InitTimer()
	{
		pTimer = std::make_unique<Timer>();
		return pTimer ? true : false;
	}
	
	void WinWindow::Loop()
	{
		MSG msg;
		ZeroMemory(&msg, sizeof(MSG));

		while (IsWindowEnabled(hwnd))
		{
			if (PeekMessage(&msg, hwnd, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
			{
				if (!PAUSEWORK)
				{
					pTimer->Tick();
					ChildLoop();
				}
			}
		}
	}

	void WinWindow::EscapeKeyProc()
	{
	}

	bool WinWindow::ISPAUSED() const {
		return PAUSEWORK;
	}

	WindowSettings WinWindow::WNDSettings() const {
		return wndSettings;
	}

	Timer* WinWindow::GetTimer() const
	{
		return pTimer.get();
	}

	LRESULT WinWindow::MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg)
		{
		case WM_ACTIVATE:
		{
			CONSOLE_MESSAGE_NO_PREF("WM_ACTIVATE ACTIVE");

			if (LOWORD(wParam) == WA_INACTIVE)
			{
				PAUSEWORK = true;
			}
			else
			{
				PAUSEWORK = false;
			}
			return 0;
		}

		case WM_KEYDOWN:

			CONSOLE_MESSAGE_NO_PREF(std::string("PRESSED KEY ID: " + std::to_string(lParam)));

			if (wParam == VK_ESCAPE)
			{
				EscapeKeyProc();
				CONSOLE_MESSAGE("ESCAPE PRESSED");
				DestroyWindow(hWnd);
			}
			else
			{
				ChildKeyPressed(wParam);
			}
			return 0;

		case WM_DESTROY:

			CONSOLE_MESSAGE("WM_DESTROY MESSAGE ACTIVE");
			PostQuitMessage(0);
			return 0;

		case WM_SIZE:
			CONSOLE_MESSAGE("WM_SIZE ACTIVE");
			SetWindowPos(hWnd, hWnd, CW_USEDEFAULT, CW_USEDEFAULT, LOWORD(lParam), HIWORD(lParam), WS_OVERLAPPEDWINDOW);
			ChildSIZE();
			return 0;

		case WM_ENTERSIZEMOVE:
		{
			CONSOLE_MESSAGE("WM_ENTERSIZE ACTIVE");
			SetWindowPos(hWnd, hWnd, CW_USEDEFAULT, CW_USEDEFAULT, LOWORD(lParam), HIWORD(lParam), WS_OVERLAPPEDWINDOW);
			ChildENTERSIZE();
			return 0;
		}
		case WM_EXITSIZEMOVE:
		{
			CONSOLE_MESSAGE("WM_EXITSIZE ACTIVE");
			SetWindowPos(hWnd, hWnd, CW_USEDEFAULT, CW_USEDEFAULT, LOWORD(lParam), HIWORD(lParam), WS_OVERLAPPEDWINDOW);
			ChildEXITSIZE();

			return 0;
		}

		case WM_MENUCHAR:
			return MAKELRESULT(0, MNC_CLOSE);

		case WM_GETMINMAXINFO:
			((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
			((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
			return 0;

		case WM_LBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_RBUTTONDOWN:
			ChildMOUSEDOWN(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			SetCapture(hWnd);
			return 0;
		case WM_LBUTTONUP:
		case WM_MBUTTONUP:
		case WM_RBUTTONUP:
			ChildMOUSEUP(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			ReleaseCapture();
			return 0;
		case WM_MOUSEMOVE:
			CONSOLE_MESSAGE_NO_PREF(std::string("MOUSE MOVED X: " + std::to_string(GET_X_LPARAM(lParam)) + " Y: " + std::to_string(GET_Y_LPARAM(lParam))));
			ChildMOUSEMOVE(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			return 0;

		}
		return DefWindowProc(hWnd,
			msg,
			wParam,
			lParam);
	}
}