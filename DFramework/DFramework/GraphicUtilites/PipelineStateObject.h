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
		
		virtual ~PipelineStateObject() = default;

		void SetRasterizerState(D3D12_RASTERIZER_DESC rasterizerDesc);
		void SetBlendState(D3D12_BLEND_DESC blendDesc);
		void SetDepthStencilState(D3D12_DEPTH_STENCIL_DESC dsvStateDesc);
		void SetSampleDesc(DXGI_SAMPLE_DESC sampleDesc);

		void SetVS(ID3DBlob* vsByteCode);
		void SetPS(ID3DBlob* psByteCode);
		void SetHS(ID3DBlob* hsByteCode);
		void SetDS(ID3DBlob* dsByteCode);
		void SetGS(ID3DBlob* gsByteCode);

		ID3D12PipelineState* GetPSO() const;
		D3D12_GRAPHICS_PIPELINE_STATE_DESC GetDesc() const;

		void CreatePSO(ID3D12Device* pDevice);

	private:


		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
		wrl::ComPtr<ID3D12PipelineState> pPSO;

	};


	class ComputePipelineStateObject
	{

	public:

		ComputePipelineStateObject() = delete;
		~ComputePipelineStateObject() = default;
		
		ComputePipelineStateObject(ID3D12RootSignature* const pRootSignature, const D3D12_PIPELINE_STATE_FLAGS flags = D3D12_PIPELINE_STATE_FLAG_NONE, const UINT nodeMask = 0);
		void SetCS(ID3DBlob* csByteCode);
		
		void CreatePSO(ID3D12Device* pDevice);
	
		ID3D12PipelineState* GetPSO() const;
		D3D12_COMPUTE_PIPELINE_STATE_DESC GetDesc() const;

	private:

		D3D12_COMPUTE_PIPELINE_STATE_DESC cPsoDesc;
		wrl::ComPtr<ID3D12PipelineState> pPSO;

	};

}