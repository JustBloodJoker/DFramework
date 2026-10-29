#pragma once

#include <Config/PipelineRegistry.h>
#include <WinWindow/Utils/CreativeSingleton.h>
#include <D3DFramework/GraphicUtilites/GraphicsPipelineObject.h>
#include <D3DFramework/GraphicUtilites/ComputePipelineObject.h>

class PSOManager : public FDWWIN::CreativeSingleton<PSOManager> {
public:

	void InitPSOjects(ID3D12Device* device);
	void InitPSOjectsDevice5(ID3D12Device5* device);

	FD3DW::BasePipelineObject* GetPSOObject(PSOType type);
	FD3DW::BasePipelineObject* GetOrCreatePSOObject(PSOType type);

	template<typename T>
	T* GetPSOObjectAs(PSOType type) {
		return dynamic_cast<T*>(GetPSOObject(type));
	}

protected:
	std::vector<std::wstring> BuildIncludeDirectories(const BasePSODescriptor& descriptor) const;
	std::unordered_map<FD3DW::CompileFileType, FD3DW::CompileDesc> BuildShaderFiles(const BasePSODescriptor& descriptor) const;
	FD3DW::BasePipelineObject* CreateComputePSOObject(PSOType type, const PSOComputeDescriptor& descriptor);
	FD3DW::BasePipelineObject* CreateRTPSOObject(PSOType type, const PSORTDescriptor& descriptor);

protected:
	std::map< PSOType, std::unique_ptr<FD3DW::BasePipelineObject>> m_mCreatedPSO;
	ID3D12Device* m_pDevice = nullptr;
	ID3D12Device5* m_pDXRDevice = nullptr;
};
