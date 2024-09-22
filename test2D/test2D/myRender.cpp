#include "myRender.h"

myRender::myRender() 
	: WinWindow(L"MaybeFD2D", 600, 600, false)
{
}

void myRender::UserAfterD2DInit()
{
	rtv = GetRenderTarget();
	brush = GetBrush();
}

void myRender::UserD2DLoop()
{
	rtv->Clear(D2D1::ColorF(D2D1::ColorF::White));
}

void myRender::UserD2DClose()
{
}
