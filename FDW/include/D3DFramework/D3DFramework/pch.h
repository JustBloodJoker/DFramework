#ifndef PCH_H
#define PCH_H


#ifdef _RELEASE
	#define NDEBUG
#endif


#include "d3dframework.h"

//////////////////////
////////	WINWINDOW

#include <WinWindow/pch.h>

#pragma comment(lib, "WinWindow.lib")

#include "Utilites/Macroses.h"
#include "Utilites/Structures.h"
#include "Utilites/UtilFunctions.h"

#endif
