#pragma once

#include <pch.h>
#include <WinWindow/Utils/CreativeSingleton.h>
#include <D3DFramework/GraphicUtilites/GraphicsPipelineObject.h>
#include <D3DFramework/GraphicUtilites/ComputePipelineObject.h>

class PSOManager : public FDWWIN::CreativeSingleton<PSOManager> {
public:

	void InitPSOjects(ID3D12Device* device);
	void InitPSOjectsDevice5(ID3D12Device5* device);

	FD3DW::BasePipelineObject* GetPSOObject(PSOType type);

	template<typename T>
	T* GetPSOObjectAs(PSOType type) {
		return dynamic_cast<T*>(GetPSOObject(type));
	}

private:
	std::map< PSOType, std::unique_ptr<FD3DW::BasePipelineObject>> m_mCreatedPSO;
};