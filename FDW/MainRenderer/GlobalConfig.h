#pragma once 

#include <pch.h>
#include <D3DFramework/GraphicUtilites/PipelineObject.h>

struct PSODescriptor {
    std::wstring shaderFolderName;
    FD3DW::GraphicPipelineObjectDesc pipelineDesc;
};

enum class PSOType {
	DefferedFirstPassAnimatedMeshesDefaultConfig,
	DefferedFirstPassSimpleMeshesDefaultConfig,
	DefferedSecondPassDefaultConfig,
};

const std::unordered_map<PSOType, PSODescriptor>& GetPSODescriptors();
const std::map<std::wstring, std::tuple<FD3DW::CompileFileType, std::wstring, std::wstring>>& GetKnownShadersData();



#define SHADERS_DEFAULT_INCLUDE_PATH L"Content/Shaders/DefaultInclude"
#define SHADERS_DEFAULT_PATH L"Content/Shaders/"
