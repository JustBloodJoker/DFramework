#pragma once 

#include <pch.h>
#include <D3DFramework/GraphicUtilites/GraphicsPipelineObject.h>
#include <D3DFramework/GraphicUtilites/ComputePipelineObject.h>
#include <D3DFramework/GraphicUtilites/RTPipelineObject.h>

//////////////////////////////////////////////
//				GBUFFERS DATA


struct GBuffersData {
	std::vector<DXGI_FORMAT> GBuffersFormats;
};

const GBuffersData& GetGBufferData();
const DXGI_FORMAT GetForwardRenderPassFormat();
UINT GetGBuffersNum();

//////////////////////////////////////////////



//////////////////////////////////////////////
//////		PSO MANAGER GLOBALS

struct PSODescriptor {
    std::wstring shaderFolderName;
    FD3DW::GraphicPipelineObjectDesc pipelineDesc;
};

struct PSORTDescriptor {
	std::wstring shaderFolderName;
	FD3DW::RayTracingPipelineConfig rtPSOConfig;
};

enum class PSOType {
	DefferedFirstPassDefaultConfig,
	DefferedSecondPassDefaultConfig,
	SimpleSkyboxDefaultConfig,
	PostProcessDefaultConfig,
	RTSoftShadowDefaultConfig,
};

const std::unordered_map<PSOType, PSODescriptor>& GetGraphicsPSODescriptors();
const std::unordered_map<PSOType, PSORTDescriptor>& GetRTPSODescriptors();
const std::map<std::wstring, std::tuple<FD3DW::CompileFileType, std::wstring, std::wstring>>& GetKnownShadersData();

#define SHADERS_DEFAULT_INCLUDE_PATH L"Content/Shaders/DefaultInclude"
#define SHADERS_DEFAULT_PATH L"Content/Shaders/"

//////////////////////////////////////////////

#define BASE_RENDERABLE_OBJECTS_BLAS_HIT_GROUP_INDEX 0

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


////////////////////////////////////////////
///////		RT SOFT SHADOWS BINDING
#define RT_SOFT_SHADOW_TLAS_BUFFER_POS_IN_ROOT_SIG				0
#define RT_SOFT_SHADOW_UAV_SHADOWS_OUT_POS_IN_ROOT_SIG			1
#define RT_SOFT_SHADOW_GBUFFERS_POS_IN_ROOT_SIG					2
#define RT_SOFT_SHADOW_LIGHTS_HELPER_BUFFER_POS_IN_ROOT_SIG		3
#define RT_SOFT_SHADOW_LIGHTS_BUFFER_POS_IN_ROOT_SIG			4

#define RT_SOFT_SHADOW_FRAME_BUFFER_POS_IN_ROOT_SIG				5
#define RT_SOFT_SHADOW_INPUT_PREV_FRAME_SRV_POS_IN_ROOT_SIG		6
#define RT_SOFT_SHADOW_OUTPUT_CURR_FRAME_UAV_POS_IN_ROOT_SIG	7
/////////////////////////////////////////////

/////////////////////////////////////////////
///////    DEFFERED SECOND PASS SRV HEAP LOCATIONS  

#define COUNT_SRV_IN_GBUFFER_HEAP 9

#define GBUFFER_POSITION_LOCATION_IN_HEAP 0
#define GBUFFER_NORMAL_LOCATION_IN_HEAP 1
#define GBUFFER_ALBEDO_LOCATION_IN_HEAP 2
#define GBUFFER_SPECULAR_LOCATION_IN_HEAP 3 
#define GBUFFER_EMISSIVE_LOCATION_IN_HEAP 4
#define GBUFFER_MATERIALDATA_LOCATION_IN_HEAP 5
#define LIGHTS_LTC_MAT_LOCATION_IN_HEAP 6
#define LIGHTS_LTC_AMP_LOCATION_IN_HEAP 7
#define SHADOW_FACTOR_LOCATION_IN_HEAP 8





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


//<<<<
//	посмотреть че за хуйня в тласе спонзы
//	починить то что пишет инвалид дескрипторы в рт
//	и поотом уже попробовать софт тени