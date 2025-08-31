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
////	GLOBAL HEAP MESHES TEXTURES

#define GLOBAL_TEXTURE_HEAP_NODE_MASK 0
#define GLOBAL_TEXTURE_HEAP_PRECACHE_SIZE 200

/////////////////////////////////////////////

//////////////////////////////////////////////
//////		PSO MANAGER GLOBALS

enum class PSOType {
	None,
	DefferedFirstPassDefaultConfig,
	DefferedFirstPassWithPreDepth,
	DefferedSecondPassDefaultConfig,
	SimpleSkyboxDefaultConfig,
	PostProcessDefaultConfig,
	RTSoftShadowDefaultConfig,
	ObjectsCullingDefaultConfig,
	PreDepthDefaultConfig,
	CopyDepthToHIZ,
	ClusteredShading_BuildGridPass,
	ClusteredShading_LightsToClusteresPass,

};

struct BasePSODescriptor {
	PSOType Parent;
	std::wstring ShaderFolderName;
	std::vector<std::wstring> AdditionalInclude = {};
};

struct PSODescriptor : public BasePSODescriptor {
    FD3DW::GraphicPipelineObjectDesc GraphicsDesc;
};

struct PSORTDescriptor : public BasePSODescriptor {
	FD3DW::RayTracingPipelineConfig RTConfig;
};

struct PSOComputeDescriptor : public BasePSODescriptor {
};

const std::unordered_map<PSOType, PSODescriptor>& GetGraphicsPSODescriptors();
const std::unordered_map<PSOType, PSORTDescriptor>& GetRTPSODescriptors();
const std::unordered_map<PSOType, PSOComputeDescriptor>& GetComputePSODescriptors();
const std::map<std::wstring, std::tuple<FD3DW::CompileFileType, std::wstring, std::wstring>>& GetKnownShadersData();

#define SHADERS_DEFAULT_INCLUDE_PATH L"Content/Shaders/DefaultInclude"
#define SHADERS_DEFAULT_PATH L"Content/Shaders/"

//////////////////////////////////////////////

#define BASE_RENDERABLE_OBJECTS_BLAS_HIT_GROUP_INDEX 0

////////////////////////////////////////////
//   PRE DEPTH PASS BINDS
#define PRE_DEPTH_CONSTANT_BUFFER_MATRICES_POSITION_IN_ROOT_SIG	0
#define PRE_DEPTH_ANIMATIONS_CONSTANT_BUFFER_IN_ROOT_SIG		1
////////////////////////////////////////////

////////////////////////////////////////////
//   CLUSTERED SHADING PASS BINDS
#define CLUSTERED_THREADS_PER_GROUP_X_1		1
#define CLUSTERED_THREADS_PER_GROUP_X_2		128

#define CLUSTERED_NUM_X_CLUSTERS			16
#define CLUSTERED_NUM_Y_CLUSTERS			9
#define CLUSTERED_NUM_Z_CLUSTERS			24

#define CLUSTERED_FIRST_PASS_CBV_CLUSTERS_PARAMS_POS_IN_ROOT_SIG		0
#define CLUSTERED_FIRST_PASS_UAV_CLUSTERS_POS_IN_ROOT_SIG				1

#define CLUSTERED_SECOND_PASS_SRV_LIGHTS_POS_IN_ROOT_SIG				0
#define CLUSTERED_SECOND_PASS_UAV_CLUSTERS_POS_IN_ROOT_SIG				1
#define CLUSTERED_SECOND_PASS_CBV_VIEWP_POS_IN_ROOT_SIG					2

////////////////////////////////////////////

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
#define LIGHTS_CLUSTERS_BUFFER_POS_IN_ROOT_SIG					3
#define LIGHTS_CLUSTERS_DATA_BUFFER_POS_IN_ROOT_SIG				4
/////////////////////////////////////////////

/////////////////////////////////////////////
//////		OBJECTS CULLING COMPUTE PASS
#define OBJECT_CULLING_OUTPUT_COMMANDS_POS_IN_PACK				0
#define OBJECT_CULLING_INPUT_DSV_POS_IN_PACK					1
#define OBJECT_CULLING_OUTPUT_UAV_HIZ_POS_IN_PACK				2
#define OBJECT_CULLING_OUTPUT_SRV_HIZ_POS_IN_PACK				3

#define OBJECT_CULLING_HIZ_COPY_DEPTH_THREAD_X_GROUP			8
#define OBJECT_CULLING_HIZ_COPY_DEPTH_THREAD_Y_GROUP			8

#define OBJECT_CULLING_HIZ_COPY_DEPTH_INPUT_POS_IN_ROOT_SIG		0	
#define OBJECT_CULLING_HIZ_COPY_DEPTH_OUTPUT_POS_IN_ROOT_SIG	1

#define OBJECT_GPU_CULLING_THREAD_GROUP_SIZE 64

#define OBJECT_GPU_CULLING_CONSTANT_BUFFER_VIEW_POS_IN_ROOT_SIG				0
#define OBJECT_GPU_CULLING_SRV_BUFFER_INSTANCES_DATA_POS_IN_ROOT_SIG		1
#define OBJECT_GPU_CULLING_SRV_BUFFER_INPUT_COMMANDS_POS_IN_ROOT_SIG		2
#define OBJECT_GPU_CULLING_HIZ_RESOURCE_POS_IN_ROOT_SIG						3
#define OBJECT_GPU_CULLING_UAV_BUFFER_OUTPUT_COMMANDS_POS_IN_ROOT_SIG		4
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