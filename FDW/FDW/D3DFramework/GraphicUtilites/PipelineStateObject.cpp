#include "../pch.h"
#include "PipelineStateObject.h"


namespace FD3DW
{





	PipelineStateObject::PipelineStateObject(ID3D12RootSignature *const pRootSignature,
												const D3D12_INPUT_ELEMENT_DESC* layout, const UINT layoutSize,
												const UINT renderTargetsNum, DXGI_FORMAT rtvFormats[],
												DXGI_FORMAT dsvFormat,
												const UINT SampleMask,
												const D3D12_PRIMITIVE_TOPOLOGY_TYPE type)
	{

		ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

		psoDesc.pRootSignature = pRootSignature;
		psoDesc.InputLayout = { layout, layoutSize };
		psoDesc.SampleMask = SampleMask;
		psoDesc.PrimitiveTopologyType = type;
		psoDesc.NumRenderTargets = renderTargetsNum;
		for (size_t ind = 0; ind < renderTargetsNum; ind++)
		{
			psoDesc.RTVFormats[ind] = rtvFormats[ind];
		}
		psoDesc.DSVFormat = dsvFormat;
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.SampleDesc.Count = 1;
		psoDesc.SampleDesc.Quality = 0;
		
		
	}

	void PipelineStateObject::SetRasterizerState(D3D12_RASTERIZER_DESC rasterizerDesc)
	{
		psoDesc.RasterizerState = rasterizerDesc;
	}

	void PipelineStateObject::SetBlendState(D3D12_BLEND_DESC blendDesc)
	{
		psoDesc.BlendState = blendDesc;
	}

	void PipelineStateObject::SetDepthStencilState(D3D12_DEPTH_STENCIL_DESC dsvStateDesc)
	{
		psoDesc.DepthStencilState = dsvStateDesc;
	}

	void PipelineStateObject::SetSampleDesc(DXGI_SAMPLE_DESC sampleDesc)
	{
		psoDesc.SampleDesc = sampleDesc;
	}

	void PipelineStateObject::SetVS(ID3DBlob* vsByteCode)
	{
		psoDesc.VS = { reinterpret_cast<BYTE*>(vsByteCode->GetBufferPointer()) , vsByteCode->GetBufferSize() };
	}

	void PipelineStateObject::SetPS(ID3DBlob* vsByteCode)
	{
		psoDesc.PS = { reinterpret_cast<BYTE*>(vsByteCode->GetBufferPointer()) , vsByteCode->GetBufferSize() };
	}

	void PipelineStateObject::SetHS(ID3DBlob* vsByteCode)
	{
		psoDesc.HS = { reinterpret_cast<BYTE*>(vsByteCode->GetBufferPointer()) , vsByteCode->GetBufferSize() };
	}

	void PipelineStateObject::SetDS(ID3DBlob* vsByteCode)
	{
		psoDesc.DS = { reinterpret_cast<BYTE*>(vsByteCode->GetBufferPointer()) , vsByteCode->GetBufferSize() };
	}

	void PipelineStateObject::SetGS(ID3DBlob* vsByteCode)
	{
		psoDesc.GS = { reinterpret_cast<BYTE*>(vsByteCode->GetBufferPointer()) , vsByteCode->GetBufferSize() };
	}

	ID3D12PipelineState* PipelineStateObject::GetPSO() const
	{
		SAFE_ASSERT(pPSO.Get(), "PSO not created. Use CreatePSO after set up PSO data");

		return pPSO.Get();
	}

	D3D12_GRAPHICS_PIPELINE_STATE_DESC PipelineStateObject::GetDesc() const
	{
		return psoDesc;
	}

	void PipelineStateObject::CreatePSO(ID3D12Device* pDevice)
	{
		HRESULT_ASSERT(pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(pPSO.GetAddressOf())), "Create PSO error");
		CONSOLE_MESSAGE("PSO created");
	}

	ComputePipelineStateObject::ComputePipelineStateObject(ID3D12RootSignature* const pRootSignature, const D3D12_PIPELINE_STATE_FLAGS flags, const UINT nodeMask )
	{
		cPsoDesc = {};
		cPsoDesc.pRootSignature = pRootSignature;
		cPsoDesc.NodeMask = nodeMask;
		cPsoDesc.Flags = flags;
	}

	void ComputePipelineStateObject::SetCS(ID3DBlob* csByteCode)
	{
		cPsoDesc.CS = { csByteCode->GetBufferPointer(), csByteCode->GetBufferSize() };
	}

	void ComputePipelineStateObject::CreatePSO(ID3D12Device* pDevice)
	{
		HRESULT_ASSERT(pDevice->CreateComputePipelineState(&cPsoDesc, IID_PPV_ARGS(pPSO.GetAddressOf())), "Create PSO error");
		CONSOLE_MESSAGE("Compute PSO created");
	}

	ID3D12PipelineState* ComputePipelineStateObject::GetPSO() const
	{
		return pPSO.Get();
	}

	D3D12_COMPUTE_PIPELINE_STATE_DESC ComputePipelineStateObject::GetDesc() const
	{
		return cPsoDesc;
	}

}