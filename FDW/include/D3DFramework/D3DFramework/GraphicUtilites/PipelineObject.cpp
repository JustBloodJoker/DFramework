#include "PipelineObject.h"

namespace FD3DW {

	static const std::vector<CompileFileType> s_vCompiledFilesForCheckRootSignature = {
		CompileFileType::RootSignature,
		CompileFileType::VS,
		CompileFileType::PS,
		CompileFileType::CS
	};

	PipelineObject::PipelineObject(ID3D12Device* device)
	{
		Initialize(device);
	}

	bool PipelineObject::Initialize(ID3D12Device* device)
	{
		m_pDevice = device;
		DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(m_pUtils.ReleaseAndGetAddressOf()));
		DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(m_pCompiler.ReleaseAndGetAddressOf()));
		m_pUtils->CreateDefaultIncludeHandler(m_pIncludeHandler.ReleaseAndGetAddressOf());
		return true;
	}

	void PipelineObject::SetIncludeDirectories(const std::vector<std::wstring>& includeDirs)
	{
		m_vIncludeDirs = includeDirs;
	}

	void PipelineObject::SetExternalRootSignature(ID3D12RootSignature* external)
	{
		m_pRootSignature = external;
		m_bExternalRootSig = true;
	}

	bool PipelineObject::CreatePSO(const std::unordered_map<CompileFileType, ShaderDesc>& shaders, const GraphicPipelineObjectDesc& config)
	{
		SAFE_ASSERT(!(HasComputeStage(shaders) && HasGraphicsStages(shaders)), "Cannot mix compute and graphics shaders in one PSO");

		std::unordered_map<CompileFileType, wrl::ComPtr<IDxcBlob>> compiledShaders;

		for (const auto& [stage, desc] : shaders) {
			if (stage == CompileFileType::RootSignature) {
				compiledShaders[stage] = CompileRootSignatureBlob(desc);
			}
			else {
				compiledShaders[stage] = CompileShader(desc);
			}

			SAFE_ASSERT(compiledShaders[stage], "Shader compilation failed");
		}

		if (!m_bExternalRootSig ) {

			for (const auto& type : s_vCompiledFilesForCheckRootSignature) {
				if (compiledShaders.contains(type))
				{
					m_pRootSignature = ExtractRootSignature(compiledShaders[type].Get());
					if (m_pRootSignature && SUCCEEDED(hr)) break;
				}
			}

			SAFE_ASSERT(m_pRootSignature, "Failed to extract root signature");
		}

		if (HasComputeStage(shaders)) {
			m_xType = PipelineType::Compute;
			D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
			desc.pRootSignature = m_pRootSignature.Get();
			desc.CS = {
				compiledShaders[CompileFileType::CS]->GetBufferPointer(),
				compiledShaders[CompileFileType::CS]->GetBufferSize()
			};
			HRESULT_ASSERT(m_pDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(m_pPSO.ReleaseAndGetAddressOf())), "CreateComputePipelineState failed");
		}
		else {
			m_xType = PipelineType::Graphics;

			if (compiledShaders.count(CompileFileType::VS)) {
				m_vInputLayout = ReflectInputLayout(compiledShaders[CompileFileType::VS].Get());
			}
			else {
				m_vInputLayout.clear();
			}

			D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
			desc.pRootSignature = m_pRootSignature.Get();
			desc.InputLayout = { m_vInputLayout.data(), (UINT)m_vInputLayout.size() };
			desc.BlendState = config.BlendState;
			desc.RasterizerState = config.RasterizerState;
			desc.DepthStencilState = config.DepthStencilState;
			desc.SampleMask = config.SampleMask;
			desc.PrimitiveTopologyType = config.TopologyType;
			desc.NumRenderTargets = config.NumRenderTargets;
			memcpy(desc.RTVFormats, config.RTVFormats, sizeof(config.RTVFormats));
			desc.DSVFormat = config.DSVFormat;
			desc.SampleDesc = config.SampleDesc;
			desc.NodeMask = config.NodeMask;

			if (compiledShaders.count(CompileFileType::VS))
				desc.VS = { compiledShaders[CompileFileType::VS]->GetBufferPointer(), compiledShaders[CompileFileType::VS]->GetBufferSize() };
			if (compiledShaders.count(CompileFileType::PS))
				desc.PS = { compiledShaders[CompileFileType::PS]->GetBufferPointer(), compiledShaders[CompileFileType::PS]->GetBufferSize() };
			if (compiledShaders.count(CompileFileType::GS))
				desc.GS = { compiledShaders[CompileFileType::GS]->GetBufferPointer(), compiledShaders[CompileFileType::GS]->GetBufferSize() };
			if (compiledShaders.count(CompileFileType::HS))
				desc.HS = { compiledShaders[CompileFileType::HS]->GetBufferPointer(), compiledShaders[CompileFileType::HS]->GetBufferSize() };
			if (compiledShaders.count(CompileFileType::DS))
				desc.DS = { compiledShaders[CompileFileType::DS]->GetBufferPointer(), compiledShaders[CompileFileType::DS]->GetBufferSize() };

			HRESULT_ASSERT(m_pDevice->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(m_pPSO.ReleaseAndGetAddressOf())), "CreateGraphicsPipelineState failed");
		}

		return true;
	}


	void PipelineObject::Bind(ID3D12GraphicsCommandList* cmdList)
	{
		cmdList->SetPipelineState(m_pPSO.Get());

		if (m_xType == PipelineType::Graphics) {
			cmdList->SetGraphicsRootSignature(m_pRootSignature.Get());
		}
		else if (m_xType == PipelineType::Compute) {
			cmdList->SetComputeRootSignature(m_pRootSignature.Get());
		}

	}

	ID3D12PipelineState* PipelineObject::GetPSO() const
	{
		return m_pPSO.Get();
	}

	ID3D12RootSignature* PipelineObject::GetRootSignature() const
	{
		return m_pRootSignature.Get();
	}

	wrl::ComPtr<IDxcBlob> PipelineObject::CompileShader(const ShaderDesc& desc)
	{
		std::ifstream file(desc.path, std::ios::binary | std::ios::ate);
		if (!file) throw std::runtime_error("Can't open shader file");

		std::vector<char> data(file.tellg());
		file.seekg(0);
		file.read(data.data(), data.size());

		DxcBuffer buffer{ data.data(), data.size(), DXC_CP_UTF8 };

		std::vector<LPCWSTR> args = {
			L"-E", desc.entry.c_str(),
			L"-T", desc.target.c_str(),
			L"-HV", L"2021",
#if defined(_DEBUG)
			L"-Zi", L"-Qembed_debug", L"-Od",
#else
			L"-O3",
#endif
		};
		
		for (const auto& dir : m_vIncludeDirs) {
			args.push_back(L"-I");
			args.push_back(dir.c_str());
		}

		wrl::ComPtr<IDxcOperationResult> result;
		m_pCompiler->Compile(&buffer, args.data(), (UINT)args.size(), m_pIncludeHandler.Get(), IID_PPV_ARGS(result.ReleaseAndGetAddressOf()));

		HRESULT_ASSERT(result->GetStatus(&hr), "Failed to retrieve shader compile status");
		if (FAILED(hr)) {
			wrl::ComPtr<IDxcBlobEncoding> errors;
			result->GetErrorBuffer(&errors);
			if (errors && errors->GetBufferSize()) {
				CONSOLE_ERROR_MESSAGE((char*)errors->GetBufferPointer());
			}
			return nullptr;
		}

		wrl::ComPtr<IDxcBlob> shaderBlob;
		result->GetResult(&shaderBlob);
		return shaderBlob;
	}

	wrl::ComPtr<IDxcBlob> PipelineObject::CompileRootSignatureBlob(const ShaderDesc& desc)
	{
		std::ifstream file(desc.path, std::ios::binary | std::ios::ate);
		if (!file) throw std::runtime_error("Can't open root signature file");

		std::vector<char> data(file.tellg());
		file.seekg(0);
		file.read(data.data(), data.size());

		DxcBuffer buffer{ data.data(), data.size(), DXC_CP_UTF8 };

		std::vector<LPCWSTR> args = {
			L"-T", desc.target.c_str(),
			L"-E", desc.entry.c_str(),
			L"-HV", L"2021",
			L"-Zi"
		};

		wrl::ComPtr<IDxcOperationResult> result;
		m_pCompiler->Compile(&buffer, args.data(), (UINT)args.size(), m_pIncludeHandler.Get(), IID_PPV_ARGS(result.ReleaseAndGetAddressOf()));

		HRESULT hr;
		result->GetStatus(&hr);
		if (FAILED(hr)) {
			wrl::ComPtr<IDxcBlobEncoding> errors;
			result->GetErrorBuffer(&errors);
			if (errors && errors->GetBufferSize()) {
				CONSOLE_ERROR_MESSAGE((char*)errors->GetBufferPointer());
			}
			return nullptr;
		}

		wrl::ComPtr<IDxcBlob> rootSigBlob;
		result->GetResult(&rootSigBlob);
		return rootSigBlob;
	}

	wrl::ComPtr<ID3D12RootSignature> PipelineObject::ExtractRootSignature(IDxcBlob* blob)
	{
		wrl::ComPtr<ID3D12RootSignature> rs;
		hr = m_pDevice->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(rs.ReleaseAndGetAddressOf()));
		return rs;
	}

	std::vector<D3D12_INPUT_ELEMENT_DESC> PipelineObject::ReflectInputLayout(IDxcBlob* blob)
	{
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

	bool PipelineObject::HasGraphicsStages(const std::unordered_map<CompileFileType, ShaderDesc>& shaders) const {
		for (CompileFileType stage : { CompileFileType::VS, CompileFileType::PS, CompileFileType::GS, CompileFileType::HS, CompileFileType::DS }) {
			if (shaders.count(stage)) return true;
		}
		return false;
	}

	bool PipelineObject::HasComputeStage(const std::unordered_map<CompileFileType, ShaderDesc>& shaders) const {
		return shaders.count(CompileFileType::CS) > 0;
	}

}


