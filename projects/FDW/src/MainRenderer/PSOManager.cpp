#include "pch.h"

#include <MainRenderer/PSOManager.h>
#include <Config/PipelineRegistry.h>



void PSOManager::InitPSOjects(ID3D12Device* device)
{
    m_pDevice = device;

    const auto& descriptors = GetGraphicsPSODescriptors();
    for (const auto& [type, descriptor] : descriptors) {
        if (descriptor.Parent == PSOType::None) {
            std::vector<std::wstring> includeData;

            std::wstring shaderDir = FDWConfig::DefaultShaderPath + descriptor.ShaderFolderName;

            includeData.push_back(FDWConfig::DefaultShaderIncludePath);
            includeData.push_back(shaderDir);

            for (const auto& addInc : descriptor.AdditionalInclude) {
                std::wstring addIncMod = FDWConfig::DefaultShaderPath + addInc;
                includeData.push_back(addIncMod);
            }

            auto pipeline = std::make_unique<FD3DW::GraphicsPipelineObject>(device);
            pipeline->SetIncludeDirectories(includeData);

            std::unordered_map<FD3DW::CompileFileType, FD3DW::CompileDesc> shaderFiles;
            const auto& knownShaders = GetKnownShadersData();
            for (const auto& [fileName, meta] : knownShaders) {
                std::filesystem::path filePath = std::filesystem::path(shaderDir) / fileName;
                if (std::filesystem::exists(filePath)) {
                    auto [type, entry, target] = meta;
                    shaderFiles[type] = { filePath.wstring(), entry, target };
                }
            }

            pipeline->SetConfig(descriptor.GraphicsDesc);
            pipeline->CreatePSO(shaderFiles);
            m_mCreatedPSO[type] = std::move(pipeline);
        }
        else {
            auto pp = GetPSOObjectAs<FD3DW::GraphicsPipelineObject>(descriptor.Parent)->MakeCopy();
            auto ptr = dynamic_cast<FD3DW::GraphicsPipelineObject*>(pp.get());
            ptr->SetConfig(descriptor.GraphicsDesc);
            ptr->CreatePSOAfterCopy();
            m_mCreatedPSO[type] = std::move(pp);

        }

    }

    const auto& computeDescriptors = GetComputePSODescriptors();
    if (computeDescriptors.empty())return;
    for (const auto& [type, descriptor] : computeDescriptors) {
        if (IsLazyPSO(type)) continue;

        std::vector<std::wstring> includeData;

        std::wstring shaderDir = FDWConfig::DefaultShaderPath + descriptor.ShaderFolderName;

        includeData.push_back(FDWConfig::DefaultShaderIncludePath);
        includeData.push_back(shaderDir);

        for (const auto& addInc : descriptor.AdditionalInclude) {
            std::wstring addIncMod = FDWConfig::DefaultShaderPath + addInc;
            includeData.push_back(addIncMod);
        }

        auto pipeline = std::make_unique<FD3DW::ComputePipelineObject>(device);
        pipeline->SetIncludeDirectories(includeData);


        std::unordered_map<FD3DW::CompileFileType, FD3DW::CompileDesc> shaderFiles;
        const auto& knownShaders = GetKnownShadersData();
        for (const auto& [fileName, meta] : knownShaders) {
            std::filesystem::path filePath = std::filesystem::path(shaderDir) / fileName;
            if (std::filesystem::exists(filePath)) {
                auto [type, entry, target] = meta;
                shaderFiles[type] = { filePath.wstring(), entry, target };
            }
        }

        pipeline->CreatePSO(shaderFiles);
        m_mCreatedPSO[type] = std::move(pipeline);
    }

}

void PSOManager::InitPSOjectsDevice5(ID3D12Device5* device)
{
    m_pDXRDevice = device;

    const auto& rtDescriptors = GetRTPSODescriptors();
    for (const auto& [type, descriptor] : rtDescriptors) {
        if (IsLazyPSO(type)) continue;

        std::vector<std::wstring> includeData;

        std::wstring shaderDir = FDWConfig::DefaultShaderPath + descriptor.ShaderFolderName;

        includeData.push_back(FDWConfig::DefaultShaderIncludePath);
        includeData.push_back(shaderDir);

        for (const auto& addInc : descriptor.AdditionalInclude) {
            std::wstring addIncMod = FDWConfig::DefaultShaderPath + addInc;
            includeData.push_back(addIncMod);
        }

        auto pipeline = std::make_unique<FD3DW::RTPipelineObject>(device);
        pipeline->SetIncludeDirectories(includeData);

        std::unordered_map<FD3DW::CompileFileType, FD3DW::CompileDesc> shaderFiles;
        const auto& knownShaders = GetKnownShadersData();
        for (const auto& [fileName, meta] : knownShaders) {
            std::filesystem::path filePath = std::filesystem::path(shaderDir) / fileName;
            if (std::filesystem::exists(filePath)) {
                auto [type, entry, target] = meta;
                shaderFiles[type] = { filePath.wstring(), entry, target };
            }
        }

        pipeline->SetConfig(descriptor.RTConfig);
        pipeline->CreatePSO(shaderFiles);
        m_mCreatedPSO[type] = std::move(pipeline);
    }
}


FD3DW::BasePipelineObject* PSOManager::GetPSOObject(PSOType type)
{
	auto iter = m_mCreatedPSO.find(type);
	if (iter == m_mCreatedPSO.end()) {
		CONSOLE_ERROR_MESSAGE("Can't find PSO with type " << int(type));
		return nullptr;
	}

	return iter->second.get();
}

FD3DW::BasePipelineObject* PSOManager::GetOrCreatePSOObject(PSOType type) {
    const auto created = m_mCreatedPSO.find(type);
    if (created != m_mCreatedPSO.end()) return created->second.get();

    const auto& computeDescriptors = GetComputePSODescriptors();
    const auto compute = computeDescriptors.find(type);
    if (compute != computeDescriptors.end()) return CreateComputePSOObject(type, compute->second);

    const auto& rtDescriptors = GetRTPSODescriptors();
    const auto rayTracing = rtDescriptors.find(type);
    if (rayTracing != rtDescriptors.end()) return CreateRTPSOObject(type, rayTracing->second);

    CONSOLE_ERROR_MESSAGE("Can't create PSO with type " << int(type));
    return nullptr;
}

std::vector<std::wstring> PSOManager::BuildIncludeDirectories(const BasePSODescriptor& descriptor) const {
    const std::wstring shaderDirectory = FDWConfig::DefaultShaderPath + descriptor.ShaderFolderName;
    std::vector<std::wstring> includeDirectories = {
        FDWConfig::DefaultShaderIncludePath,
        shaderDirectory
    };

    for (const auto& additionalInclude : descriptor.AdditionalInclude) {
        includeDirectories.push_back(FDWConfig::DefaultShaderPath + additionalInclude);
    }

    return includeDirectories;
}

std::unordered_map<FD3DW::CompileFileType, FD3DW::CompileDesc> PSOManager::BuildShaderFiles(const BasePSODescriptor& descriptor) const {
    const std::filesystem::path shaderDirectory = std::filesystem::path(FDWConfig::DefaultShaderPath) / descriptor.ShaderFolderName;

    std::unordered_map<FD3DW::CompileFileType, FD3DW::CompileDesc> shaderFiles;

    for (const auto& [fileName, metadata] : GetKnownShadersData()) {
        const auto filePath = shaderDirectory / fileName;
        if (!std::filesystem::exists(filePath)) continue;

        const auto& [type, entryPoint, target] = metadata;
        shaderFiles[type] = {filePath.wstring(), entryPoint, target};
    }

    return shaderFiles;
}

FD3DW::BasePipelineObject* PSOManager::CreateComputePSOObject(PSOType type, const PSOComputeDescriptor& descriptor) {
    if (!m_pDevice) {
        CONSOLE_ERROR_MESSAGE("Can't create compute PSO before PSOManager device initialization");
        return nullptr;
    }

    auto pipeline = std::make_unique<FD3DW::ComputePipelineObject>(m_pDevice);
    pipeline->SetIncludeDirectories(BuildIncludeDirectories(descriptor));
    if (!pipeline->CreatePSO(BuildShaderFiles(descriptor))) {
        CONSOLE_ERROR_MESSAGE("Can't compile compute PSO with type " << int(type));
        return nullptr;
    }

    auto* result = pipeline.get();
    m_mCreatedPSO[type] = std::move(pipeline);
    return result;
}

FD3DW::BasePipelineObject* PSOManager::CreateRTPSOObject(PSOType type, const PSORTDescriptor& descriptor) {
    if (!m_pDXRDevice) {
        CONSOLE_ERROR_MESSAGE("Can't create ray-tracing PSO before PSOManager DXR device initialization");
        return nullptr;
    }

    auto pipeline = std::make_unique<FD3DW::RTPipelineObject>(m_pDXRDevice);
    pipeline->SetIncludeDirectories(BuildIncludeDirectories(descriptor));
    pipeline->SetConfig(descriptor.RTConfig);
    if (!pipeline->CreatePSO(BuildShaderFiles(descriptor))) {
        CONSOLE_ERROR_MESSAGE("Can't compile ray-tracing PSO with type " << int(type));
        return nullptr;
    }

    auto* result = pipeline.get();
    m_mCreatedPSO[type] = std::move(pipeline);
    return result;
}
