#pragma once

#include "WinWindow/Utils/Macroses.h"

#define VERSION_D3DFRAMEWORK 1

#ifndef BUFFERS_COUNT
#define BUFFERS_COUNT 2
#endif

#ifndef NUM_BONES_PER_VEREX
#define NUM_BONES_PER_VEREX 13
#endif

#define M_PI_2_F 1.57079632679489661923f

									
#ifdef _XBOX 
#define fourccRIFF 'RIFF'
#define fourccDATA 'data'
#define fourccFMT 'fmt '
#define fourccWAVE 'WAVE'
#else
#define fourccRIFF 'FFIR'
#define fourccDATA 'atad'
#define fourccFMT  ' tmf'
#define fourccWAVE 'EVAW'
#endif
