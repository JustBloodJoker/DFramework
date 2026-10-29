#pragma once

#include <Config/PipelineRegistry.h>
#include <WinWindow/Utils/CreativeSingleton.h>
#include <D3DFramework/GraphicUtilites/GraphicsPipelineObject.h>
#include <D3DFramework/GraphicUtilites/ComputePipelineObject.h>

class PSOManager : public FDWWIN::CreativeSingleton<PSOManager> {
public:
	void InitializePSOs(ID3D12Device* device);
	void InitializeRayTracingPSOs(ID3D12Device5* device);

	FD3DW::BasePipelineObject* GetPSOObject(PSOType type);

	template<typename T>
	T* GetPSOObjectAs(PSOType type) {
		return dynamic_cast<T*>(GetPSOObject(type));
	}

protected:
	std::vector<std::wstring> BuildIncludeDirectories(const BasePSODescriptor& descriptor) const;
	std::unordered_map<FD3DW::CompileFileType, FD3DW::CompileDesc> BuildShaderFiles(
		const BasePSODescriptor& descriptor,
		bool enable16BitTypes = false) const;

protected:
	std::map<PSOType, std::unique_ptr<FD3DW::BasePipelineObject>> m_mCreatedPSO;
};
