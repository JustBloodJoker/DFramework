#pragma once
#include "BasePipelineObject.h"

namespace FD3DW {

	struct GraphicPipelineObjectDesc {
		CD3DX12_BLEND_DESC BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		CD3DX12_RASTERIZER_DESC RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		CD3DX12_DEPTH_STENCIL_DESC DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		D3D12_PRIMITIVE_TOPOLOGY_TYPE TopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		DXGI_FORMAT RTVFormats[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
		UINT NumRenderTargets = 0;
		DXGI_FORMAT DSVFormat = DXGI_FORMAT_UNKNOWN;
		DXGI_SAMPLE_DESC SampleDesc = { 1, 0 };
		UINT SampleMask = UINT_MAX;
		UINT NodeMask = 0;
	};

	class GraphicsPipelineObject : public BasePipelineObject {
	public:
		GraphicsPipelineObject(ID3D12Device* device);

		bool CreatePSO(const std::unordered_map<CompileFileType, CompileDesc>& shaders) override;
		void Bind(ID3D12GraphicsCommandList* cmdList) override;
		std::unique_ptr<BasePipelineObject> MakeCopy() const override;

		void SetConfig(const GraphicPipelineObjectDesc& config);
		GraphicPipelineObjectDesc GetConfig() const;

	private:
		std::vector<D3D12_INPUT_ELEMENT_DESC> ReflectInputLayout(IDxcBlob* blob);
		GraphicPipelineObjectDesc m_xConfig;
		std::vector<D3D12_INPUT_ELEMENT_DESC> m_vInputLayout;
	};

}
