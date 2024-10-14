#pragma once
#include "../pch.h"

#define _DEFINE_EASY_DFW
#ifdef _DEFINE_EASY_DFW

#include "../D3DFW.h"

using namespace FD3DW;

class EasyD3DFW : public FD3DW::D3DFW {
public:
	EasyD3DFW() = delete;
	virtual ~EasyD3DFW() = default;
	EasyD3DFW(std::wstring windowTittle, int width, int height, bool fullScreen);

public:
	void DrawToRenderTarget(RenderTarget* rtvOut, DepthStencilView* dsvOut, std::function<void(void)> drawHandler);


};


#endif // _DEFINE_EASY_DFW
