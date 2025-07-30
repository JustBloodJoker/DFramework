#pragma once 

#include <pch.h>
#include <D3DFramework/GraphicUtilites/GraphicsPipelineObject.h>
#include <D3DFramework/GraphicUtilites/ComputePipelineObject.h>

//////////////////////////////////////////////
//				GBUFFERS DATA


struct GBuffersData {
	std::vector<DXGI_FORMAT> GBuffersFormats;
};

const GBuffersData& GetGBufferData();
const DXGI_FORMAT GetForwardRenderPassFormat();
const UINT& GetGBuffersNum();

//////////////////////////////////////////////



//////////////////////////////////////////////
//////		PSO MANAGER GLOBALS

struct PSODescriptor {
    std::wstring shaderFolderName;
    FD3DW::GraphicPipelineObjectDesc pipelineDesc;
};

enum class PSOType {
	DefferedFirstPassDefaultConfig,
	DefferedSecondPassDefaultConfig,
	SimpleSkyboxDefaultConfig,
	PostProcessDefaultConfig,
};

const std::unordered_map<PSOType, PSODescriptor>& GetGraphicsPSODescriptors();
const std::map<std::wstring, std::tuple<FD3DW::CompileFileType, std::wstring, std::wstring>>& GetKnownShadersData();

#define SHADERS_DEFAULT_INCLUDE_PATH L"Content/Shaders/DefaultInclude"
#define SHADERS_DEFAULT_PATH L"Content/Shaders/"

//////////////////////////////////////////////


////////////////////////////////////////////
//   DEFFERED FIRST PASS BINDS
#define CONSTANT_BUFFER_MATRICES_POSITION_IN_ROOT_SIG			0
#define CONSTANT_BUFFER_MATERIALS_POSITION_IN_ROOT_SIG			1
#define TEXTURE_START_POSITION_IN_ROOT_SIG						2
#define ANIMATIONS_CONSTANT_BUFFER_IN_ROOT_SIG					3
////////////////////////////////////////////

////////////////////////////////////////////
///////		DEFFERED SECOND PASS BINDS
#define DEFFERED_GBUFFERS_POS_IN_ROOT_SIG						0
#define LIGHTS_HELPER_BUFFER_POS_IN_ROOT_SIG					1
#define LIGHTS_BUFFER_POS_IN_ROOT_SIG							2
/////////////////////////////////////////////


/////////////////////////////////////////////
///////    DEFFERED SECOND PASS SRV HEAP LOCATIONS  

#define COUNT_SRV_IN_GBUFFER_HEAP 8

#define GBUFFER_POSITION_LOCATION_IN_HEAP 0
#define GBUFFER_NORMAL_LOCATION_IN_HEAP 1
#define GBUFFER_ALBEDO_LOCATION_IN_HEAP 2
#define GBUFFER_SPECULAR_LOCATION_IN_HEAP 3 
#define GBUFFER_EMISSIVE_LOCATION_IN_HEAP 4
#define GBUFFER_MATERIALDATA_LOCATION_IN_HEAP 5
#define LIGHTS_LTC_MAT_LOCATION_IN_HEAP 6
#define LIGHTS_LTC_AMP_LOCATION_IN_HEAP 7





void InitializeDescriptorSizes(ID3D12Device* device, UINT rtvDescSize, UINT dsvDescSize, UINT cbv_srv_uavDescSize);
const UINT& GetDSVDescriptorSize(ID3D12Device* device);
const UINT& GetRTVDescriptorSize(ID3D12Device* device);
const UINT& GetCBV_SRV_UAVDescriptorSize(ID3D12Device* device);

////////////////////////////////////////////

////////////////////////////////////////////
#define SCENES_DEFAULT_PATH L"Content/"

////////////////////////////////////////////


////////////////////////////////////////////
#define LIGHTS_LTC_TEXTURES_PATH_MAT "Content/LTC/ltc_mat.dds"
#define LIGHTS_LTC_TEXTURES_PATH_AMP "Content/LTC/ltc_amp.dds"


////////////////////////////////////////////