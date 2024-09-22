#pragma once
#include "pch.h"
#include "WinWindow/WinWindow.h"

namespace FD2DW
{

	class D2DFW : virtual public FDWWIN::WinWindow
	{
	public:
		D2DFW()=default;
		virtual ~D2DFW() =default;

	public:


		virtual void UserAfterD2DInit() =0;
		virtual void UserD2DLoop() =0;
		virtual void UserD2DClose() =0;

	protected:
		virtual bool InitD2D() = 0;
		virtual void ChildBeforeUserLoop() = 0;
		virtual void ChildAfterUserLoop() = 0;

	private:
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