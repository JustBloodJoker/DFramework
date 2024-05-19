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

#define _USE_MATH_DEFINES 
#include <cmath>

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
#include <wrl/client.h>

//DIRECT3D12
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#include "include/d3dx12.h"
#include <DirectXMath.h>


//NAMESPACES
namespace wrl = Microsoft::WRL;
namespace dx = DirectX;