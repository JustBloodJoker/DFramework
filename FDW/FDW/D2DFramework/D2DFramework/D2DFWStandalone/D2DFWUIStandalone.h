#pragma once
#include "pch.h"

#include "D2DFW_UIController.h"
#include "D2DFWStandalone/D2DFWStandalone.h"


namespace FD2DW {

	class D2DFWUIStandalone : virtual public D2DFW_UIController, virtual public D2DFWStandalone {
	public:
		D2DFWUIStandalone() = default;
		virtual ~D2DFWUIStandalone() = default;

		virtual void ChildMOUSEUP(WPARAM btnState, int x, int y) override;
		virtual void ChildMOUSEDOWN(WPARAM btnState, int x, int y) override;
		virtual void ChildMOUSEMOVE(WPARAM btnState, int x, int y) override;
		virtual void ChildKeyPressed(WPARAM key) override;

		virtual ID2D1RenderTarget* GetRenderTarget() const override;
		ID2D1SolidColorBrush* GetBrush() const override;

	public:

		//do nothing here
		virtual void UserD2DClose() override;

	private:

		virtual void UserAfterD2DInit() override final;
		virtual void UserD2DLoop() override final;

	};


}