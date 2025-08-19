#pragma once

#include "../../pch.h"
#include "RenderThreadManager.h"
#include "WinWindow/Utils/CreativeSingleton.h"

namespace FD3DW {

	class GlobalRenderThreadManager : virtual public RenderThreadManager, virtual public FDWWIN::CreativeSingleton<GlobalRenderThreadManager> {
	public:

		GlobalRenderThreadManager() = default;
		virtual ~GlobalRenderThreadManager() = default;

	};


}
