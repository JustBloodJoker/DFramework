#pragma once
#include "pch.h"
#include <D3DFramework/D3DFW.h>
#include "D2DFWDXGI.h"

#pragma warning(disable : 4250)

namespace FDW {

	using namespace FD2DW;
	using namespace FD3DW;

	class DFW : virtual public D3DFW, virtual public D2DFWDDXGI {
	public:

		DFW() = default;
		virtual ~DFW() = default;

	private:
		/////////////////////
		// METHODS FROM D2D
		// NOW PRIVATE FOR TEST
		virtual void UserAfterD2DInit() override;
		virtual void UserD2DLoop() override;
		virtual void UserD2DClose() override;
		virtual bool InitD2D() override;
		virtual void ChildBeforeUserLoop() override;
		virtual void ChildAfterUserLoop() override;




	};

}
