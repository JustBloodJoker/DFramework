#pragma once 

#include <pch.h>
#include <D3DFramework/GraphicUtilites/PipelineObject.h>

//////////////////////////////////////////////
//////		PSO MANAGER GLOBALS

///////////////////
//   MESHES
#define CONSTANT_BUFFER_MATRICES_POSITION_IN_ROOT_SIG 0
#define CONSTANT_BUFFER_MATERIALS_POSITION_IN_ROOT_SIG 1
#define TEXTURE_START_POSITION_IN_ROOT_SIG 2
#define ANIMATIONS_CONSTANT_BUFFER_IN_ROOT_SIG 3




struct PSODescriptor {
    std::wstring shaderFolderName;
    FD3DW::GraphicPipelineObjectDesc pipelineDesc;
};

enum class PSOType {
	DefferedFirstPassDefaultConfig,
	DefferedSecondPassDefaultConfig,
	SimpleSkyboxDefaultConfig
};

const std::unordered_map<PSOType, PSODescriptor>& GetPSODescriptors();
const std::map<std::wstring, std::tuple<FD3DW::CompileFileType, std::wstring, std::wstring>>& GetKnownShadersData();

#define SHADERS_DEFAULT_INCLUDE_PATH L"Content/Shaders/DefaultInclude"
#define SHADERS_DEFAULT_PATH L"Content/Shaders/"

//////////////////////////////////////////////

/////////////////////////////////////////////

void InitializeDescriptorSizes(ID3D12Device* device, UINT rtvDescSize, UINT dsvDescSize, UINT cbv_srv_uavDescSize);
const UINT& GetDSVDescriptorSize(ID3D12Device* device);
const UINT& GetRTVDescriptorSize(ID3D12Device* device);
const UINT& GetCBV_SRV_UAVDescriptorSize(ID3D12Device* device);

////////////////////////////////////////////

////////////////////////////////////////////
#define SCENES_DEFAULT_PATH L"Content/"

////////////////////////////////////////////


