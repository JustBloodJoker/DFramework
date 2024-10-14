#ifndef  _DISABLE_D2DFW_STANDALONE

#pragma once
#include "pch.h"
#include "D2DFW.h"

namespace FD2DW
{

	class D2DFWStandalone : virtual public D2DFW
	{
	public:
		D2DFWStandalone() = default;
		virtual ~D2DFWStandalone() = default;

	protected:
		//////////////////////////////
		////		GETTERS
		ID2D1Factory*				GetFactory()		const noexcept;
		ID2D1HwndRenderTarget*		GetRenderTarget()	const noexcept;
		ID2D1SolidColorBrush*		GetBrush()			const noexcept;
		RECT						GetMainRect()		const noexcept;

		//////////////////////////////
		//			FACADE
		void SetMainBrushColor(D2D1::ColorF color);
		void SetBrushColor(ID2D1SolidColorBrush* brush, D2D1::ColorF color);

	private:
		///////////////////////////
		virtual void ChildBeforeUserLoop() override;
		virtual void ChildAfterUserLoop() override;
		virtual bool InitD2D() override;
		///////////////////////////

		///////////////////////////
		void EndDrawMainRenderTarget();
		///////////////////////////
	

	private:
		virtual bool ChildInit() override;
		virtual void ChildLoop() override;
		virtual void ChildRelease() override;
		virtual void ChildKeyPressed(WPARAM) override;
		virtual void ChildSIZE() override;
		virtual void ChildENTERSIZE() override;
		virtual void ChildEXITSIZE() override;
		virtual void ChildMOUSEUP(WPARAM btnState, int x, int y) override;
		virtual void ChildMOUSEMOVE(WPARAM btnState, int x, int y) override;
		virtual void ChildMOUSEDOWN(WPARAM btnState, int x, int y) override;

	private:
		wrl::ComPtr<ID2D1Factory> pFactory;
		wrl::ComPtr<ID2D1HwndRenderTarget> pRenderTarget;
		wrl::ComPtr<ID2D1SolidColorBrush> pBrush;

		RECT mainRect;
	};

}

#endif // ! _DISABLE_D2DFW_STANDALONE