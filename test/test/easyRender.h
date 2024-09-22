#pragma once
#define _DEFINE_EASY_DFW
#include "EasyD3DFW/EasyD3DFW.h"
#include "EasyD3DFW/EasyMacroses.h"

class easyRender : public EasyD3DFW
{
public:
	easyRender();
	virtual ~easyRender()=default;
	DECL_STANDART_D3DFW_OVERRIDED_METHODS();



};

