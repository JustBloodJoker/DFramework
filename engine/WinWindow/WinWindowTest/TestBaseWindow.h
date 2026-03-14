#pragma once
#include "WinWindow/WinWindow.h"


class TestBaseWindow : public FDWWIN::WinWindow
{
public:
	TestBaseWindow(std::wstring windowTittle, int width, int height, bool fullScreen);

	virtual bool ChildInit() override;
	virtual void ChildLoop() override;
	virtual void ChildRelease() override;

};

