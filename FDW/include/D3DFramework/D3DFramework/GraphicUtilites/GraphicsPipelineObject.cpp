#include "GraphicsPipelineObject.h"

namespace FD3DW {
	GraphicsPipelineObject::GraphicsPipelineObject(ID3D12Device* device) :BasePipelineObject(device) {}

	bool GraphicsPipelineObject::CreatePSO(const std::unordered_map<CompileFileType, CompileDesc>& shaders) {
		SAFE_ASSERT(HasGraphicsStages(shaders), "Cannot create graphics pipelone object - missing VS shader");

		CompileShaders(shaders);
		ExtractRootSignature();

		if (m_mCompiledShaders.count(CompileFileType::VS)) {
			m_vInputLayout = ReflectInputLayout(m_mCompiledShaders[CompileFileType::VS].Get());
		}
		else {
			m_vInputLayout.clear();
		}

		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
		desc.pRootSignature = m_pRootSignature.Get();
		desc.InputLayout = { m_vInputLayout.data(), (UINT)m_vInputLayout.size() };
		desc.BlendState = m_xConfig.BlendState;
		desc.RasterizerState = m_xConfig.RasterizerState;
		desc.DepthStencilState = m_xConfig.DepthStencilState;
		desc.SampleMask = m_xConfig.SampleMask;
		desc.PrimitiveTopologyType = m_xConfig.TopologyType;
		desc.NumRenderTargets = m_xConfig.NumRenderTargets;
		memcpy(desc.RTVFormats, m_xConfig.RTVFormats, sizeof(m_xConfig.RTVFormats));
		desc.DSVFormat = m_xConfig.DSVFormat;
		desc.SampleDesc = m_xConfig.SampleDesc;
		desc.NodeMask = m_xConfig.NodeMask;

		if (m_mCompiledShaders.count(CompileFileType::VS))
			desc.VS = { m_mCompiledShaders[CompileFileType::VS]->GetBufferPointer(), m_mCompiledShaders[CompileFileType::VS]->GetBufferSize() };
		if (m_mCompiledShaders.count(CompileFileType::PS))
			desc.PS = { m_mCompiledShaders[CompileFileType::PS]->GetBufferPointer(), m_mCompiledShaders[CompileFileType::PS]->GetBufferSize() };
		if (m_mCompiledShaders.count(CompileFileType::GS))
			desc.GS = { m_mCompiledShaders[CompileFileType::GS]->GetBufferPointer(), m_mCompiledShaders[CompileFileType::GS]->GetBufferSize() };
		if (m_mCompiledShaders.count(CompileFileType::HS))
			desc.HS = { m_mCompiledShaders[CompileFileType::HS]->GetBufferPointer(), m_mCompiledShaders[CompileFileType::HS]->GetBufferSize() };
		if (m_mCompiledShaders.count(CompileFileType::DS))
			desc.DS = { m_mCompiledShaders[CompileFileType::DS]->GetBufferPointer(), m_mCompiledShaders[CompileFileType::DS]->GetBufferSize() };

		HRESULT_ASSERT(m_pDevice->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(m_pPSO.ReleaseAndGetAddressOf())), "CreateGraphicsPipelineState failed");

		return true;
	}
	void GraphicsPipelineObject::Bind(ID3D12GraphicsCommandList* cmdList) {
		cmdList->SetPipelineState(m_pPSO.Get());
		cmdList->SetGraphicsRootSignature(m_pRootSignature.Get());
	}
	std::unique_ptr<BasePipelineObject> GraphicsPipelineObject::MakeCopy() const {
		if (m_mCompiledShaders.empty()) {
			CONSOLE_ERROR_MESSAGE("Graphics Pipeline Object copy failed: shaders not compiled");
			return nullptr;
		}

		std::unique_ptr<GraphicsPipelineObject> copy = std::make_unique<GraphicsPipelineObject>(m_pDevice);
		copy->m_xConfig = m_xConfig;
		copy->m_mCompiledShaders = this->m_mCompiledShaders;
		if (m_bExternalRootSig) copy->SetExternalRootSignature(m_pRootSignature.Get());

		return copy;
	}

	void GraphicsPipelineObject::SetConfig(const GraphicPipelineObjectDesc& config) {
		m_xConfig = config;
	}
	GraphicPipelineObjectDesc GraphicsPipelineObject::GetConfig() const {
		return m_xConfig;
	}


	std::vector<D3D12_INPUT_ELEMENT_DESC> GraphicsPipelineObject::ReflectInputLayout(IDxcBlob* blob) {
		wrl::ComPtr<IDxcContainerReflection> container;
		UINT32 idx;
		wrl::ComPtr<ID3D12ShaderReflection> reflection;
		DxcCreateInstance(CLSID_DxcContainerReflection, IID_PPV_ARGS(container.ReleaseAndGetAddressOf()));

		HRESULT_ASSERT(container->Load(blob), "Failed to load container reflection");
		HRESULT_ASSERT(container->FindFirstPartKind(DXC_PART_DXIL, &idx), "DXIL part not found");
		HRESULT_ASSERT(container->GetPartReflection(idx, IID_PPV_ARGS(reflection.ReleaseAndGetAddressOf())), "Failed to get shader reflection");

		D3D12_SHADER_DESC shaderDesc;
		reflection->GetDesc(&shaderDesc);


		std::vector<D3D12_INPUT_ELEMENT_DESC> layout;
		for (UINT i = 0; i < shaderDesc.InputParameters; ++i) {
			D3D12_SIGNATURE_PARAMETER_DESC param;
			reflection->GetInputParameterDesc(i, &param);

			DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
			switch (param.Mask) {
			case 1: format = DXGI_FORMAT_R32_FLOAT; break;
			case 3: format = DXGI_FORMAT_R32G32_FLOAT; break;
			case 7: format = DXGI_FORMAT_R32G32B32_FLOAT; break;
			case 15: format = DXGI_FORMAT_R32G32B32A32_FLOAT; break;
			}

			if (std::string(param.SemanticName) == std::string("SV_INSTANCEID")) continue;

			layout.push_back({
				_strdup(param.SemanticName),
				param.SemanticIndex,
				format,
				0,
				D3D12_APPEND_ALIGNED_ELEMENT,
				D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
				0
				});
		}

		return layout;

	}


}