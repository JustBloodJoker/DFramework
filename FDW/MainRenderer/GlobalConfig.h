#pragma once 

#include <pch.h>
#include <D3DFramework/GraphicUtilites/PipelineObject.h>

//////////////////////////////////////////////
//////		PSO MANAGER GLOBALS

struct PSODescriptor {
    std::wstring shaderFolderName;
    FD3DW::GraphicPipelineObjectDesc pipelineDesc;
};

enum class PSOType {
	DefferedFirstPassAnimatedMeshesDefaultConfig,
	DefferedFirstPassSimpleMeshesDefaultConfig,
	DefferedSecondPassDefaultConfig,
	SimpleSkyboxDefaultConfig,
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


