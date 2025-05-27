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

		ZeroMemory(&m_xPSODesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

		m_xPSODesc.pRootSignature = pRootSignature;
		m_xPSODesc.InputLayout = { layout, layoutSize };
		m_xPSODesc.SampleMask = SampleMask;
		m_xPSODesc.PrimitiveTopologyType = type;
		m_xPSODesc.NumRenderTargets = renderTargetsNum;
		for (size_t ind = 0; ind < renderTargetsNum; ind++)
		{
			m_xPSODesc.RTVFormats[ind] = rtvFormats[ind];
		}
		m_xPSODesc.DSVFormat = dsvFormat;
		m_xPSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		m_xPSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		m_xPSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		m_xPSODesc.SampleDesc.Count = 1;
		m_xPSODesc.SampleDesc.Quality = 0;
		
		
	}

	void PipelineStateObject::SetRasterizerState(D3D12_RASTERIZER_DESC rasterizerDesc)
	{
		m_xPSODesc.RasterizerState = rasterizerDesc;
	}

	void PipelineStateObject::SetBlendState(D3D12_BLEND_DESC blendDesc)
	{
		m_xPSODesc.BlendState = blendDesc;
	}

	void PipelineStateObject::SetDepthStencilState(D3D12_DEPTH_STENCIL_DESC dsvStateDesc)
	{
		m_xPSODesc.DepthStencilState = dsvStateDesc;
	}

	void PipelineStateObject::SetSampleDesc(DXGI_SAMPLE_DESC sampleDesc)
	{
		m_xPSODesc.SampleDesc = sampleDesc;
	}

	void PipelineStateObject::SetVS(ID3DBlob* vsByteCode)
	{
		m_xPSODesc.VS = { reinterpret_cast<BYTE*>(vsByteCode->GetBufferPointer()) , vsByteCode->GetBufferSize() };
	}

	void PipelineStateObject::SetPS(ID3DBlob* vsByteCode)
	{
		m_xPSODesc.PS = { reinterpret_cast<BYTE*>(vsByteCode->GetBufferPointer()) , vsByteCode->GetBufferSize() };
	}

	void PipelineStateObject::SetHS(ID3DBlob* vsByteCode)
	{
		m_xPSODesc.HS = { reinterpret_cast<BYTE*>(vsByteCode->GetBufferPointer()) , vsByteCode->GetBufferSize() };
	}

	void PipelineStateObject::SetDS(ID3DBlob* vsByteCode)
	{
		m_xPSODesc.DS = { reinterpret_cast<BYTE*>(vsByteCode->GetBufferPointer()) , vsByteCode->GetBufferSize() };
	}

	void PipelineStateObject::SetGS(ID3DBlob* vsByteCode)
	{
		m_xPSODesc.GS = { reinterpret_cast<BYTE*>(vsByteCode->GetBufferPointer()) , vsByteCode->GetBufferSize() };
	}

	ID3D12PipelineState* PipelineStateObject::GetPSO() const
	{
		SAFE_ASSERT(m_pPSO.Get(), "PSO not created. Use CreatePSO after set up PSO data");

		return m_pPSO.Get();
	}

	D3D12_GRAPHICS_PIPELINE_STATE_DESC PipelineStateObject::GetDesc() const
	{
		return m_xPSODesc;
	}

	void PipelineStateObject::CreatePSO(ID3D12Device* pDevice)
	{
		HRESULT_ASSERT(pDevice->CreateGraphicsPipelineState(&m_xPSODesc, IID_PPV_ARGS(m_pPSO.GetAddressOf())), "Create PSO error");
		CONSOLE_MESSAGE("PSO created");
	}

	ComputePipelineStateObject::ComputePipelineStateObject(ID3D12RootSignature* const pRootSignature, const D3D12_PIPELINE_STATE_FLAGS flags, const UINT nodeMask )
	{
		m_xPSODesc = {};
		m_xPSODesc.pRootSignature = pRootSignature;
		m_xPSODesc.NodeMask = nodeMask;
		m_xPSODesc.Flags = flags;
	}

	void ComputePipelineStateObject::SetCS(ID3DBlob* csByteCode)
	{
		m_xPSODesc.CS = { csByteCode->GetBufferPointer(), csByteCode->GetBufferSize() };
	}

	void ComputePipelineStateObject::CreatePSO(ID3D12Device* pDevice)
	{
		HRESULT_ASSERT(pDevice->CreateComputePipelineState(&m_xPSODesc, IID_PPV_ARGS(m_pPSO.GetAddressOf())), "Create PSO error");
		CONSOLE_MESSAGE("Compute PSO created");
	}

	ID3D12PipelineState* ComputePipelineStateObject::GetPSO() const
	{
		return m_pPSO.Get();
	}

	D3D12_COMPUTE_PIPELINE_STATE_DESC ComputePipelineStateObject::GetDesc() const
	{
		return m_xPSODesc;
	}

}