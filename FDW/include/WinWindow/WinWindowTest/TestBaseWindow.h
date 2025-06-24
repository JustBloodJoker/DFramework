#pragma once
#include "WinWindow.h"


class TestBaseWindow : public FDWWIN::WinWindow
{
public:
	TestBaseWindow(std::wstring windowTittle, int width, int height, bool fullScreen);

	virtual bool ChildInit() override;
	virtual void ChildLoop() override;
	virtual void ChildRelease() override;
	virtual void ChildKeyPressed(WPARAM) override;
	virtual void ChildSIZE() override;
	virtual void ChildENTERSIZE() override;
	virtual void ChildEXITSIZE() override;
	virtual void ChildMOUSEUP(WPARAM btnState, int x, int y) override;
	virtual void ChildMOUSEDOWN(WPARAM btnState, int x, int y) override;
	virtual void ChildMOUSEMOVE(WPARAM btnState, int x, int y) override;

};

