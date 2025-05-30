#pragma once
#define VERSION_WINWINDOW 0

static HRESULT hr = S_OK;

#ifdef _DEBUG

#define HRESULT_ASSERT(cond, text) { hr = cond; if(FAILED(hr)){ std::cerr << text << " HR CODE " << std::hex << hr << std::endl; assert(false && true);}  }
#define SAFE_ASSERT(cond, text) { if(!cond) { std::cerr << text << std::endl; assert(false && true);	} }

#define CONSOLE_MESSAGE(text) { std::clog << "MESSAGE: " << text << std::endl; }
#define CONSOLE_ERROR_MESSAGE(text) { std::cerr << "ERROR: " << text << std::endl; }
#define CONSOLE_MESSAGE_NO_PREF(text) { std::clog << text << std::endl; }


#else

#define SAFE_ASSERT(cond, text) { cond; }
#define HRESULT_ASSERT(cond, text) { hr = cond; }
#define CONSOLE_MESSAGE(text) { }
#define CONSOLE_ERROR_MESSAGE(text) { }
#define CONSOLE_MESSAGE_NO_PREF(text) { }

#endif


#define _CRT_SECURE_NO_WARNIGNS
#define PRINTF_FDW(text) printf("\n %s \n", text);