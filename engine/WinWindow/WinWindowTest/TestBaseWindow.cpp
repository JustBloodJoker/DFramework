#include "TestBaseWindow.h"

TestBaseWindow::TestBaseWindow(std::wstring windowTittle, int width, int height, bool fullScreen) :
    FDWWIN::WinWindow(windowTittle, width, height, fullScreen)
{
}

bool TestBaseWindow::ChildInit()
{
    return false;
}

void TestBaseWindow::ChildLoop() 
{
}

void TestBaseWindow::ChildRelease() 
{
}
