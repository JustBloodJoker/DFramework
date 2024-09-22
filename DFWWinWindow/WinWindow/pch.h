#pragma once


#define _XM_NO_INTRINSICS_
#include <iostream>
#include <cassert>
#include <string>
#include <memory>
#include <fstream>
#include <tuple>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <set>
#include <random>
#include <chrono>

#define _USE_MATH_DEFINES 
#include <cmath>

//MULTITHREAD
#include <thread>
#include <future>
#include <mutex>

///////////////////////
///////		WINAPI WINDOW
#include <Windows.h>
#include <Windowsx.h>
#include <winbase.h>
#include <wrl/client.h>

#include "Utils/Macroses.h"
#include "Utils/WindowSettings.h"



namespace wrl = Microsoft::WRL;