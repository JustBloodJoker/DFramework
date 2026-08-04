#include "TestBaseWindow.h"

TestBaseWindow::TestBaseWindow(std::wstring windowTittle, int width, int height, bool fullScreen) :
    FDWWIN::WinWindow(windowTittle, width, height, fullScreen)
{
}

bool TestBaseWindow::ChildInit()
{
	PostMessage(GETHWND(), WM_CLOSE, 0, 0);
	return true;
}

void TestBaseWindow::ChildLoop() 
{
}

void TestBaseWindow::ChildRelease() 
{
}
