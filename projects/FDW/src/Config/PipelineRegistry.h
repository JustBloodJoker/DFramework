#pragma once

#include <D3DFramework/GraphicUtilites/GraphicsPipelineObject.h>
#include <D3DFramework/GraphicUtilites/ComputePipelineObject.h>
#include <D3DFramework/GraphicUtilites/RTPipelineObject.h>

enum class PSOType {
	None,
	DefferedFirstPassDefaultConfig,
	DefferedFirstPassWithPreDepth,
	DefferedSecondPassDefaultConfig,
	SimpleSkyboxDefaultConfig,
	PostProcessDefaultConfig,
	ObjectsCullingDefaultConfig,
	PreDepthDefaultConfig,
	CopyDepthToHIZ,
	ClusteredShading_BuildGridPass,
	ClusteredShading_LightsToClusteresPass,
	NeuralShadowWorkGeneration,
	NeuralShadowBaseline,
	NeuralShadowRayTracing,
	NeuralShadowTensorPacking,
	BloomEffect_BrightPass,
	BloomEffect_GaussianBlurPass,
	BloomEffect_CompositePass,
	IBL_IrradianceConvolution,
	IBL_PrefilterConvolution,
	GPUSceneSkinning,
	AtlasRTShadowDefaultConfig,
	PerLightRTShadowDefaultConfig,
	TAAPass
};

struct BasePSODescriptor {
	PSOType Parent;
	std::wstring ShaderFolderName;
	std::vector<std::wstring> AdditionalInclude = {};
};

struct PSODescriptor : BasePSODescriptor {
	FD3DW::GraphicPipelineObjectDesc GraphicsDesc;
};

struct PSORTDescriptor : BasePSODescriptor {
	FD3DW::RayTracingPipelineConfig RTConfig;
};

struct PSOComputeDescriptor : BasePSODescriptor {
};

const std::unordered_map<PSOType, PSODescriptor>& GetGraphicsPSODescriptors();
const std::unordered_map<PSOType, PSORTDescriptor>& GetRTPSODescriptors();
const std::unordered_map<PSOType, PSOComputeDescriptor>& GetComputePSODescriptors();
const std::map<std::wstring, std::tuple<FD3DW::CompileFileType, std::wstring, std::wstring>>& GetKnownShadersData();
bool IsLazyPSO(PSOType type);

namespace FDWConfig {
inline constexpr wchar_t DefaultShaderIncludePath[] = L"Content/Shaders/DefaultInclude";
inline constexpr wchar_t DefaultShaderPath[] = L"Content/Shaders/";
}
