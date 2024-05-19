#pragma once

#include "../pch.h"


namespace FDW
{


	class PipelineStateObject
	{

		
	public:
		PipelineStateObject() = delete;

		PipelineStateObject(ID3D12RootSignature* const pRootSignature,
			const D3D12_INPUT_ELEMENT_DESC* layout, const UINT layoutSize,
			const UINT renderTargetsNum, DXGI_FORMAT rtvFormats[],
			DXGI_FORMAT dsvFormat,
			const UINT SampleMask = UINT_MAX,
			const D3D12_PRIMITIVE_TOPOLOGY_TYPE type = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		
		~PipelineStateObject() = default;

		void SetRasterizerState(D3D12_RASTERIZER_DESC rasterizerDesc);
		void SetBlendState(D3D12_BLEND_DESC blendDesc);
		void SetDepthStencilState(D3D12_DEPTH_STENCIL_DESC dsvStateDesc);
		void SetSampleDesc(DXGI_SAMPLE_DESC sampleDesc);

		void SetVS(ID3DBlob* vsByteCode);
		void SetPS(ID3DBlob* vsByteCode);
		void SetHS(ID3DBlob* vsByteCode);
		void SetDS(ID3DBlob* vsByteCode);
		void SetGS(ID3DBlob* vsByteCode);

		ID3D12PipelineState* GetPSO() const;
		D3D12_GRAPHICS_PIPELINE_STATE_DESC GetDesc() const;

		void CreatePSO(ID3D12Device* pDevice);

	private:


		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
		wrl::ComPtr<ID3D12PipelineState> pPSO;

	};


}