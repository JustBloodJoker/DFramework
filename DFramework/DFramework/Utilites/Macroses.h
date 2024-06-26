#pragma once

#define VERSION 0

#ifndef BUFFERS_COUNT
#define BUFFERS_COUNT 2
#endif

#ifndef NUM_BONES_PER_VEREX
#define NUM_BONES_PER_VEREX 13
#endif

static HRESULT hr = S_OK;

#define SAFE_ASSERT(cond, text) { if(!cond) { std::cerr << text << std::endl; assert(false && true);	} }
#define HRESULT_ASSERT(cond, text) { hr = cond; if(FAILED(hr)){ std::cerr << text << " HR CODE " << std::hex << hr << std::endl; assert(false && true);}  }

#define M_PI_2_F 1.57079632679489661923f

#ifdef _DEBUG
	
#define CONSOLE_MESSAGE(text) { std::clog << "MESSAGE: " << text << std::endl; }
#define CONSOLE_ERROR_MESSAGE(text) { std::cerr << "ERROR: " << text << std::endl; }
#define CONSOLE_MESSAGE_NO_PREF(text) { std::clog << text << std::endl; }

#else

#define CONSOLE_MESSAGE(text) { }
#define CONSOLE_MESSAGE_NO_PREF(text) { }

#endif

