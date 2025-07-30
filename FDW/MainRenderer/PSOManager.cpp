#include <MainRenderer/PSOManager.h>



void PSOManager::InitPSOjects(ID3D12Device* device)
{
    const auto& descriptors = GetGraphicsPSODescriptors();

    for (const auto& [type, descriptor] : descriptors) {
        std::wstring shaderDir = SHADERS_DEFAULT_PATH + descriptor.shaderFolderName;

        auto pipeline = std::make_unique<FD3DW::GraphicsPipelineObject>(device);
        pipeline->SetIncludeDirectories({ SHADERS_DEFAULT_INCLUDE_PATH, shaderDir });
        
        std::unordered_map<FD3DW::CompileFileType, FD3DW::CompileDesc> shaderFiles;
        const auto& knownShaders = GetKnownShadersData();
        for (const auto& [fileName, meta] : knownShaders) {
            std::filesystem::path filePath = std::filesystem::path(shaderDir) / fileName;
            if (std::filesystem::exists(filePath)) {
                auto [type, entry, target] = meta;
                shaderFiles[type] = { filePath.wstring(), entry, target };
            }
        }

        pipeline->SetConfig(descriptor.pipelineDesc);
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
