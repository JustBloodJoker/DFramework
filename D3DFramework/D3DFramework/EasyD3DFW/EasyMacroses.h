#pragma once

#define DECL_STANDART_D3DFW_OVERRIDED_METHODS() 						\
	virtual void UserInit() override;									\
	virtual void UserLoop() override;									\
	virtual void UserClose() override;									\
	virtual void UserMouseDown(WPARAM btnState, int x, int y) override;	\
	virtual void UserMouseUp(WPARAM btnState, int x, int y) override;	\
	virtual void UserMouseMoved(WPARAM btnState, int x, int y) override;\
	virtual void UserKeyPressed(WPARAM wParam) override;				\
	virtual void UserResizeUpdate() override;							\
