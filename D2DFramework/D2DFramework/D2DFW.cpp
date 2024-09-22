#include "pch.h"
#include "D2DFW.h"


namespace FD2DW
{



	bool D2DFW::ChildInit()
	{
		if (InitD2D()) {
			CONSOLE_MESSAGE("D2D INITED");
		}

		CONSOLE_MESSAGE("CALL USER INIT");
		UserAfterD2DInit();

		return true;
	}

	void D2DFW::ChildLoop()
	{
		ChildBeforeUserLoop();
		UserD2DLoop();
		ChildAfterUserLoop();
	}

	void D2DFW::ChildRelease()
	{
		UserD2DClose();
		CONSOLE_MESSAGE("USER CLOSED");
	}

	void D2DFW::ChildKeyPressed(WPARAM param)
	{
	}

	void D2DFW::ChildSIZE()
	{

	}

	void D2DFW::ChildENTERSIZE()
	{

	}

	void D2DFW::ChildEXITSIZE()
	{
	}

	void D2DFW::ChildMOUSEUP(WPARAM btnState, int x, int y)
	{
	}

	void D2DFW::ChildMOUSEMOVE(WPARAM btnState, int x, int y)
	{
	}

	void D2DFW::ChildMOUSEDOWN(WPARAM btnState, int x, int y)
	{
	}
}