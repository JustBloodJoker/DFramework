#pragma once

#include <pch.h>
#include <D3DFramework/GraphicUtilites/RenderThreadUtils/RenderThreadManager.h>
#include <WinWindow/Utils/CreativeSingleton.h>

class GlobalRenderThreadManager : virtual public FD3DW::RenderThreadManager, virtual public FDWWIN::CreativeSingleton<GlobalRenderThreadManager> {
public:

	GlobalRenderThreadManager();
	virtual ~GlobalRenderThreadManager() = default;

};
