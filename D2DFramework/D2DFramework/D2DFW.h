#pragma once
#include "pch.h"
#include "WinWindow/WinWindow.h"

namespace FD2DW
{

	class D2DFW : virtual public FDWWIN::WinWindow
	{
	public:
		D2DFW()=default;
		virtual ~D2DFW() = default;

	public:
		///////////////////////////
		//		WINWINDOW
		
	protected:




	protected:
		virtual bool ChildInit() override;
		virtual void ChildLoop() override;
		virtual void ChildRelease() override;
		virtual void ChildKeyPressed(WPARAM) override;
		virtual void ChildSIZE() override;
		virtual void ChildENTERSIZE() override;
		virtual void ChildEXITSIZE() override;
		virtual void ChildMOUSEUP(WPARAM btnState, int x, int y) override;
		virtual void ChildMOUSEMOVE(WPARAM btnState, int x, int y) override;
		virtual void ChildMOUSEDOWN(WPARAM btnState, int x, int y) override;

	};

}