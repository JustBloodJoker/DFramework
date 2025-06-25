#include "pch.h"
#include "WinWindow.h"

namespace FDWWIN
{
	LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		WinWindow* pThis = nullptr;

		if (msg == WM_NCCREATE)
		{
			CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
			pThis = reinterpret_cast<WinWindow*>(cs->lpCreateParams);
			SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
			pThis->SETHWND(hWnd);
		}
		else
		{
			pThis = reinterpret_cast<WinWindow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
		}

		if (pThis)
		{
			return pThis->MsgProc(hWnd, msg, wParam, lParam);
		}

		return DefWindowProc(hWnd, msg, wParam, lParam);
	}

	WinWindow::WinWindow(std::wstring windowTittle, int Width, int height, bool FullScreen)
	{
		m_xWndSettings.FullScreen = FullScreen;
		m_xWndSettings.Height = height;
		m_xWndSettings.Width = Width;
		m_xWndSettings.TittleName = windowTittle;

		m_bPAUSEWORK = false;
	}

	void WinWindow::__START()
	{
		if (m_bIsStartedWindow) return;

		m_bIsStartedWindow = true;

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

	void WinWindow::SETHWND(HWND hwnd)
	{
		this->m_xHWND = hwnd;
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

		HINSTANCE hInstance = GetModuleHandle(nullptr);

		wc.cbSize = sizeof(WNDCLASSEX);
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = WndProc;
		wc.cbClsExtra = NULL;
		wc.cbWndExtra = NULL;
		wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hInstance = hInstance;
		wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 2);
		wc.lpszMenuName = NULL;
		std::wstring className = L"wndClass";
		wc.lpszClassName = className.c_str();
		wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

		if (!GetClassInfoEx(GetModuleHandle(nullptr), L"wndClass", &wc))
		{
			if (!RegisterClassEx(&wc)) {
				DWORD err = GetLastError();
				std::wcerr << L"RegisterClassEx failed. Error: " << err << std::endl;
				SAFE_ASSERT(false, "RegisterClassEx failed");
			}
		}
		
		RECT rect = { 0, 0, m_xWndSettings.Width, m_xWndSettings.Height };
		AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

		m_xHWND = CreateWindowEx(
			0,
			L"wndClass",
			this->m_xWndSettings.TittleName.c_str(),
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT,
			rect.right - rect.left, rect.bottom - rect.top,
			NULL,
			NULL,
			hInstance,
			this
		);

		if (!m_xHWND) {
			DWORD err = GetLastError();
			std::wcerr << L"CreateWindowEx failed. Error: " << err << std::endl;
		}

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
				if (m_bPAUSEWORK)
				{
					WaitMessage();
				}
				else
				{
					m_pTimer->Tick();
					ChildLoop();
				}
			}
		}
	}

	void WinWindow::ChildAllMSG(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) { }

	void WinWindow::EscapeKeyProc()
	{
	}

	bool WinWindow::ISPAUSED() const {
		return m_bPAUSEWORK;
	}

	bool WinWindow::ISSTARTEDWINDOW() const {
		return m_bIsStartedWindow;
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
		bool isHandled = false;
		
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
			isHandled = true;
			break;
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
			isHandled = true;
			break;

		case WM_DESTROY:

			CONSOLE_MESSAGE("WM_DESTROY MESSAGE ACTIVE");
			PostQuitMessage(0);
			isHandled = true;
			break;

		case WM_SIZE:
			CONSOLE_MESSAGE("WM_SIZE ACTIVE");
			ChildSIZE();
			isHandled = true;
			break;

		case WM_ENTERSIZEMOVE:
		{
			CONSOLE_MESSAGE("WM_ENTERSIZE ACTIVE");
			
			ChildENTERSIZE();
			isHandled = true;
			break;
		}
		case WM_EXITSIZEMOVE:
		{
			CONSOLE_MESSAGE("WM_EXITSIZE ACTIVE");
			
			RECT clientRect;
			GetClientRect(hWnd, &clientRect);
			int clientWidth = clientRect.right - clientRect.left;
			int clientHeight = clientRect.bottom - clientRect.top;
			
			m_xWndSettings.Width = clientWidth;
			m_xWndSettings.Height = clientHeight;

			ChildEXITSIZE();
			isHandled = true;
			break;
		}

		case WM_MENUCHAR:
			return MAKELRESULT(0, MNC_CLOSE);

		case WM_GETMINMAXINFO:
			((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
			((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
			isHandled = true;
			break;

		case WM_LBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_RBUTTONDOWN:
			ChildMOUSEDOWN(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			SetCapture(hWnd);
			isHandled = true;
			break;
		case WM_LBUTTONUP:
		case WM_MBUTTONUP:
		case WM_RBUTTONUP:
			ChildMOUSEUP(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			ReleaseCapture();
			isHandled = true;
			break;
		case WM_MOUSEMOVE:
			CONSOLE_MESSAGE_NO_PREF(std::string("MOUSE MOVED X: " + std::to_string(GET_X_LPARAM(lParam)) + " Y: " + std::to_string(GET_Y_LPARAM(lParam))));
			ChildMOUSEMOVE(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			isHandled = true;
			break;

		}
		
		ChildAllMSG(hWnd, msg, wParam, lParam);



		return isHandled ? 0 : DefWindowProc(hWnd, msg, wParam, lParam);
	}

}