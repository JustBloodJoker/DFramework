#include "pch.h"
#include "WinWindow.h"

namespace FDWWIN 
{
	WinWindow* WinWindow::s_pInstance = nullptr;

	WinWindow* WinWindow::GetWINInstance()
	{
		return s_pInstance;
	}

	LRESULT WinWindow::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		return WinWindow::GetWINInstance()->MsgProc(hWnd, msg, wParam, lParam);
	}
	
	WinWindow::WinWindow(std::wstring windowTittle, int Width, int height, bool FullScreen)
	{
		if (!WinWindow::s_pInstance)
		{
			WinWindow::s_pInstance = this;
		}

		m_xWndSettings.FullScreen = FullScreen;
		m_xWndSettings.Height = height;
		m_xWndSettings.Width = Width;
		m_xWndSettings.TittleName = windowTittle;

		m_bPAUSEWORK = false;
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

	void WinWindow::SETHWND(HWND& m_xHWND)
	{
		m_xHWND = this->m_xHWND;
	}

	HWND WinWindow::GETHWND() const
	{
		return m_xHWND;
	}

	void WinWindow::HideCMD()
	{
		HWND Stealth;
		AllocConsole();
		Stealth = FindWindowA("ConsoleWindowClass", NULL);
		ShowWindow(Stealth, 0);
	}

	void WinWindow::Release() {
		ChildRelease();

		m_pTimer.release();
		CONSOLE_MESSAGE("WINWINDOW RELEASE");
	}

	void WinWindow::SetFullScreen()
	{
		if (!m_xWndSettings.FullScreen)
		{
			m_xWndSettings.FullScreen = true;
			HMONITOR hmon = MonitorFromWindow(m_xHWND,
				MONITOR_DEFAULTTONEAREST);
			MONITORINFO mi = { sizeof(mi) };
			GetMonitorInfo(hmon, &mi);
			m_xWndSettings.Width = mi.rcMonitor.right - mi.rcMonitor.left;
			m_xWndSettings.Height = mi.rcMonitor.bottom - mi.rcMonitor.top;
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

		m_xHWND = CreateWindowEx(NULL,
			L"wndClass",
			this->m_xWndSettings.TittleName.c_str(),
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT,
			this->m_xWndSettings.Width, this->m_xWndSettings.Height,
			NULL,
			NULL,
			NULL,
			NULL);

		SAFE_ASSERT(m_xHWND, "Create m_xHWND error");

		ShowWindow(m_xHWND, 1);
		UpdateWindow(m_xHWND);
		return true;
	}

	bool WinWindow::InitTimer()
	{
		m_pTimer = std::make_unique<Timer>();
		return m_pTimer ? true : false;
	}
	
	void WinWindow::Loop()
	{
		MSG msg;
		ZeroMemory(&msg, sizeof(MSG));

		while (IsWindowEnabled(m_xHWND))
		{
			if (PeekMessage(&msg, m_xHWND, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
			{
				if (!m_bPAUSEWORK)
				{
					m_pTimer->Tick();
					ChildLoop();
				}
			}
		}
	}

	void WinWindow::EscapeKeyProc()
	{
	}

	bool WinWindow::ISPAUSED() const {
		return m_bPAUSEWORK;
	}

	WindowSettings WinWindow::WNDSettings() const {
		return m_xWndSettings;
	}

	Timer* WinWindow::GetTimer() const
	{
		return m_pTimer.get();
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
				m_bPAUSEWORK = true;
			}
			else
			{
				m_bPAUSEWORK = false;
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