#pragma once

#include <chrono>

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
#include <numeric>
#include <queue>

#define _USE_MATH_DEFINES 
#include <cmath>

//MULTITHREAD
#include <thread>
#include <future>
#include <mutex>

//ASSIMP
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/material.h>
#include <assimp/texture.h>
#include <assimp/anim.h>
#include <assimp/light.h>
#include <assimp/postprocess.h>

//WINAPI
#include <Windows.h>
#include <Windowsx.h>
#include <winbase.h>
#include <wrl/client.h>

//DIRECT3D12
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxcompiler.lib")
#pragma comment(lib, "cuda.lib")
#pragma comment(lib, "cudart.lib")

#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3d12shader.h>
#include <dxcapi.h>
#include "include/d3dx12.h"
#include <DirectXMath.h>

// SOUND
#include <xaudio2.h>
#include <xaudio2fx.h>

//NAMESPACES
namespace wrl = Microsoft::WRL;
namespace dx = DirectX;