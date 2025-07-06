#pragma once
#include "pch.h"

#include "Utils/Timer.h"
#include "InputSystem/WinWindowInputRouter.h"

namespace FDWWIN
{
	LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	class WinWindow
	{
		friend LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
		LRESULT CALLBACK MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	public:

		WinWindow()=default;
		WinWindow(std::wstring windowTittle, int width, int height, bool fullScreen);
		virtual ~WinWindow() = default;

		virtual void __START() final;

		void SETHWND(HWND hwnd);
		HWND GETHWND() const;
		WindowSettings WNDSettings() const;
		WinWindowInputRouter* GetInputRouter() const;

	protected:
		
		void HideCMD();

		//////////////////////////
		////////	CHILD INITS
		virtual bool ChildInit() = 0;

		//////////////////////////
		///////		CHILD LOOPS
		virtual void ChildLoop() = 0;
		
		//////////////////////////
		///////		CHILD RELEASE
		virtual void ChildRelease() = 0;

	protected:
		virtual void EscapeKeyProc();

	protected:
		//////////////////
		///		GETTERS
		bool ISPAUSED() const;
		bool ISSTARTEDWINDOW() const;
		Timer* GetTimer() const;

	private:
		void SetFullScreen();
		void Loop();
		void Release();

		bool InitWindow();
		bool InitTimer();
		bool InitInputRouter();

		////////////////
		//	WINDOW
		WindowSettings m_xWndSettings;
		HWND m_xHWND;
		bool m_bPAUSEWORK = false;

		bool m_bIsStartedWindow = false;

		///////////////
		//	TIMER
		std::unique_ptr <Timer> m_pTimer;

		///////////////
		// INPUT
		std::unique_ptr<WinWindowInputRouter> m_pInputRouter;

	};

}