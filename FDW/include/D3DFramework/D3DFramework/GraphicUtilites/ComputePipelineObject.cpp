#include "ComputePipelineObject.h"

namespace FD3DW {



ComputePipelineObject::ComputePipelineObject(ID3D12Device* device) : BasePipelineObject(device) {}

bool ComputePipelineObject::CreatePSOAfterCopy(){
	return false;
}

bool ComputePipelineObject::CreatePSO(const std::unordered_map<CompileFileType, CompileDesc>& shaders) {
	
	SAFE_ASSERT(HasComputeStage(shaders), "Compute pipeline object doesn't have CS shader");

	CompileShaders(shaders);
	ExtractRootSignature();

	D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
	desc.pRootSignature = m_pRootSignature.Get();
	desc.CS = {
		m_mCompiledShaders[CompileFileType::CS]->GetBufferPointer(),
		m_mCompiledShaders[CompileFileType::CS]->GetBufferSize()
	};
	HRESULT_ASSERT(m_pDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(m_pPSO.ReleaseAndGetAddressOf())), "CreateComputePipelineState failed");
	return true;
}

void ComputePipelineObject::Bind(ID3D12GraphicsCommandList* cmdList) {
	cmdList->SetPipelineState(m_pPSO.Get());
	cmdList->SetComputeRootSignature(m_pRootSignature.Get());
}

std::unique_ptr<BasePipelineObject> ComputePipelineObject::MakeCopy() const {
	CONSOLE_ERROR_MESSAGE("ComputePipelineObject copy failed: compute PSO type is not supported (only graphics PSO is allowed).");
	return nullptr;
}


}
