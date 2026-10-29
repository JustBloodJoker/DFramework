#include <pch.h>

#include <Config/PipelineRegistry.h>
#include <Renderer/PSOManager.h>

void PSOManager::InitializePSOs(ID3D12Device* device) {
	for (const auto& [type, descriptor] : GetGraphicsPSODescriptors()) {
		if (descriptor.Parent != PSOType::None) {
			auto* parent = GetPSOObjectAs<FD3DW::GraphicsPipelineObject>(descriptor.Parent);
			if (!parent) {
				CONSOLE_ERROR_MESSAGE("Can't create copied graphics PSO without its parent");
				continue;
			}

			auto pipeline = parent->MakeCopy();
			auto* graphicsPipeline = dynamic_cast<FD3DW::GraphicsPipelineObject*>(pipeline.get());
			if (!graphicsPipeline) {
				CONSOLE_ERROR_MESSAGE("Can't cast copied graphics PSO");
				continue;
			}

			graphicsPipeline->SetConfig(descriptor.GraphicsDesc);
			if (!graphicsPipeline->CreatePSOAfterCopy()) {
				CONSOLE_ERROR_MESSAGE("Can't create copied graphics PSO with type " << static_cast<int>(type));
				continue;
			}

			m_mCreatedPSO[type] = std::move(pipeline);
			continue;
		}

		auto pipeline = std::make_unique<FD3DW::GraphicsPipelineObject>(device);
		pipeline->SetIncludeDirectories(BuildIncludeDirectories(descriptor));
		pipeline->SetConfig(descriptor.GraphicsDesc);

		if (!pipeline->CreatePSO(BuildShaderFiles(descriptor))) {
			CONSOLE_ERROR_MESSAGE("Can't create graphics PSO with type " << static_cast<int>(type));
			continue;
		}

		m_mCreatedPSO[type] = std::move(pipeline);
	}

	for (const auto& [type, descriptor] : GetComputePSODescriptors()) {
		auto pipeline = std::make_unique<FD3DW::ComputePipelineObject>(device);
		pipeline->SetIncludeDirectories(BuildIncludeDirectories(descriptor));

		if (!pipeline->CreatePSO(BuildShaderFiles(descriptor, descriptor.Enable16BitTypes))) {
			CONSOLE_ERROR_MESSAGE("Can't create compute PSO with type " << static_cast<int>(type));
			continue;
		}

		m_mCreatedPSO[type] = std::move(pipeline);
	}
}

void PSOManager::InitializeRayTracingPSOs(ID3D12Device5* device) {
	for (const auto& [type, descriptor] : GetRTPSODescriptors()) {
		auto pipeline = std::make_unique<FD3DW::RTPipelineObject>(device);
		pipeline->SetIncludeDirectories(BuildIncludeDirectories(descriptor));
		pipeline->SetConfig(descriptor.RTConfig);

		if (!pipeline->CreatePSO(BuildShaderFiles(descriptor))) {
			CONSOLE_ERROR_MESSAGE("Can't create ray-tracing PSO with type " << static_cast<int>(type));
			continue;
		}

		m_mCreatedPSO[type] = std::move(pipeline);
	}
}

FD3DW::BasePipelineObject* PSOManager::GetPSOObject(PSOType type) {
	const auto iter = m_mCreatedPSO.find(type);
	if (iter == m_mCreatedPSO.end()) {
		CONSOLE_ERROR_MESSAGE("Can't find PSO with type " << static_cast<int>(type));
		return nullptr;
	}

	return iter->second.get();
}

std::vector<std::wstring> PSOManager::BuildIncludeDirectories(const BasePSODescriptor& descriptor) const {
	const std::wstring shaderDirectory = NSOfflineConfig::DefaultShaderPath + descriptor.ShaderFolderName;
	std::vector<std::wstring> includeDirectories = {
		NSOfflineConfig::DefaultShaderIncludePath,
		shaderDirectory
	};

	for (const auto& additionalInclude : descriptor.AdditionalInclude) {
		includeDirectories.push_back(NSOfflineConfig::DefaultShaderPath + additionalInclude);
	}

	return includeDirectories;
}

std::unordered_map<FD3DW::CompileFileType, FD3DW::CompileDesc> PSOManager::BuildShaderFiles(const BasePSODescriptor& descriptor,  bool enable16BitTypes) const {
	
	const std::filesystem::path shaderDirectory = std::filesystem::path(NSOfflineConfig::DefaultShaderPath) / descriptor.ShaderFolderName;
	std::unordered_map<FD3DW::CompileFileType, FD3DW::CompileDesc> shaderFiles;

	for (const auto& [fileName, metadata] : GetKnownShadersData()) {
		
		const auto filePath = shaderDirectory / fileName;
		if (!std::filesystem::exists(filePath)) {
			continue;
		}

		const auto& [type, entryPoint, target] = metadata;
		const bool enableStage16BitTypes = enable16BitTypes && type != FD3DW::CompileFileType::RootSignature;
		
		shaderFiles[type] = { filePath.wstring(), entryPoint, target, true, enableStage16BitTypes };
	}

	return shaderFiles;
}
