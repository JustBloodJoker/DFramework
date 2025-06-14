#pragma once
#include "pch.h"
#include <D3DFramework/D3DFW.h>
#include "D2DFWDXGI.h"

#pragma warning(disable : 4250)

namespace FDW {

	using namespace FD2DW;
	using namespace FD3DW;

	class DFW : virtual public D3DFW, virtual public D2DFW_DXGI_UIController {
	public:

		DFW() = default;
		virtual ~DFW() = default;

	private:



	};

}
