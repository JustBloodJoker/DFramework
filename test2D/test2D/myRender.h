#pragma once
#include <D2DFWStandalone.h>.h>

class myRender :
	public FD2DW::D2DFWStandalone
{
public:
	myRender();
	virtual ~myRender()=default;

protected:
	virtual void UserAfterD2DInit() override;
	virtual void UserD2DLoop() override;
	virtual void UserD2DClose() override;

private:
	ID2D1HwndRenderTarget* rtv = nullptr;
	ID2D1SolidColorBrush* brush = nullptr;

};

