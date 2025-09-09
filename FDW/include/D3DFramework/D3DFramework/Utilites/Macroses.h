#pragma once

#include "WinWindow/Utils/Macroses.h"

#define VERSION_D3DFRAMEWORK 1

#ifndef BUFFERS_COUNT
#define BUFFERS_COUNT 2
#endif

#define _DEFAULT_MATERIAL_NOT_LOADED_DATA -11.0f

#ifndef DEFAULT_SWAPCHAIN_RTV_TYPE
#define DEFAULT_SWAPCHAIN_RTV_TYPE DXGI_FORMAT_R8G8B8A8_UNORM
#endif

#ifndef DEFAULT_INDEX_BUFFER_FORMAT
#define DEFAULT_INDEX_BUFFER_FORMAT DXGI_FORMAT_R32_UINT
#endif

#ifndef DEFAULT_RT_VERTEX_BUFFER_FORMAT
#define DEFAULT_RT_VERTEX_BUFFER_FORMAT DXGI_FORMAT_R32G32B32_FLOAT
#endif

#ifndef NUM_BONES_PER_VEREX
#define NUM_BONES_PER_VEREX 4
#endif

#define M_PI_2_F 1.5707963f
#define M_PI_F 3.141592654f
#define M_2_PI_F 6.283185307f

									
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
