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

void TestBaseWindow::ChildKeyPressed(WPARAM) 
{
}

void TestBaseWindow::ChildSIZE()
{
}

void TestBaseWindow::ChildENTERSIZE()
{
}

void TestBaseWindow::ChildEXITSIZE()
{
}

void TestBaseWindow::ChildMOUSEUP(WPARAM btnState, int x, int y)
{
}

void TestBaseWindow::ChildMOUSEDOWN(WPARAM btnState, int x, int y)
{
}

void TestBaseWindow::ChildMOUSEMOVE(WPARAM btnState, int x, int y)
{
}
