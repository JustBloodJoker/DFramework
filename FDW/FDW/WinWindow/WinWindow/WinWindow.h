#pragma once
#include "pch.h"

#include "Utils/Timer.h"

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

		//////////////////////////
		///////		CHILD HANDLERS
		virtual void ChildKeyPressed(WPARAM) = 0;
		virtual void ChildSIZE() = 0;
		virtual void ChildENTERSIZE() = 0;
		virtual void ChildEXITSIZE() = 0;
		virtual void ChildMOUSEUP(WPARAM btnState, int x, int y) = 0;
		virtual void ChildMOUSEDOWN(WPARAM btnState, int x, int y) = 0;
		virtual void ChildMOUSEMOVE(WPARAM btnState, int x, int y) = 0;


	protected:
		virtual void EscapeKeyProc();

	protected:
		//////////////////
		///		GETTERS
		bool ISPAUSED() const;
		bool ISSTARTEDWINDOW() const;
		WindowSettings WNDSettings() const;
		Timer* GetTimer() const;

	private:
		void SetFullScreen();
		void Loop();
		void Release();

		bool InitWindow();
		bool InitTimer();

		////////////////
		//	WINDOW
		WindowSettings m_xWndSettings;
		HWND m_xHWND;
		bool m_bPAUSEWORK = false;

		bool m_bIsStartedWindow = false;

		///////////////
		//	TIMER
		std::unique_ptr <Timer> m_pTimer;
	};

}