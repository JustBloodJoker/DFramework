#pragma once
#include "BasePipelineObject.h"

namespace FD3DW {

	class ComputePipelineObject : public BasePipelineObject {
	public:
		ComputePipelineObject(ID3D12Device* device);

		bool CreatePSO(const std::unordered_map<CompileFileType, CompileDesc>& shaders) override;
		void Bind(ID3D12GraphicsCommandList* cmdList) override;
		std::unique_ptr<BasePipelineObject> MakeCopy() const override;
	};

}
