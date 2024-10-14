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
	};

}