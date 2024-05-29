#pragma once

#define VERSION 0

#ifndef BUFFERS_COUNT
#define BUFFERS_COUNT 2
#endif

static HRESULT hr = S_OK;

#define SAFE_ASSERT(cond, text) { if(!cond) { assert(false && text);	} }
#define HRESULT_ASSERT(cond, text) { hr = cond; if(FAILED(hr)){ std::cerr << text << " HR CODE " << std::hex << hr << std::endl; assert(false && text);}  }


#ifdef _DEBUG
	
#define CONSOLE_MESSAGE(text) { std::clog << "MESSAGE: " << text << std::endl; }
#define CONSOLE_MESSAGE_NO_PREF(text) { std::clog << text << std::endl; }

#else

#define CONSOLE_MESSAGE(text) { }
#define CONSOLE_MESSAGE_NO_PREF(text) { }

#endif

