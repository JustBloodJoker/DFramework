#include "PipelineObject.h"
#include <dxcapi.h>

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

		for (const auto& [stage, desc] : shaders) {
			if (stage == CompileFileType::RootSignature) {
				m_mCompiledShaders[stage] = CompileRootSignatureBlob(desc);
			}
			else {
				m_mCompiledShaders[stage] = CompileShader(desc);
			}

			SAFE_ASSERT(m_mCompiledShaders[stage], "Shader compilation failed");
		}

		if (HasComputeStage(shaders)) {
			m_xType = PipelineType::Compute;
		}
		else {
			m_xType = PipelineType::Graphics;
		}

		m_xConfig = config;

		return CreatePSO();
	}


	bool PipelineObject::CreatePSO() {
		if (!m_bExternalRootSig) {
			for (const auto& type : s_vCompiledFilesForCheckRootSignature) {
				if (m_mCompiledShaders.contains(type))
				{
					m_pRootSignature = ExtractRootSignature(m_mCompiledShaders[type].Get());
					if (m_pRootSignature && SUCCEEDED(hr)) break;
				}
			}

			SAFE_ASSERT(m_pRootSignature, "Failed to extract root signature");
		}

		if (m_xType == PipelineType::Compute) {
			D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
			desc.pRootSignature = m_pRootSignature.Get();
			desc.CS = {
				m_mCompiledShaders[CompileFileType::CS]->GetBufferPointer(),
				m_mCompiledShaders[CompileFileType::CS]->GetBufferSize()
			};
			HRESULT_ASSERT(m_pDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(m_pPSO.ReleaseAndGetAddressOf())), "CreateComputePipelineState failed");
		}
		else {
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

	GraphicPipelineObjectDesc PipelineObject::GetGraphicPSOConfig() const
	{
		return m_xConfig;
	}

	std::unique_ptr<PipelineObject> PipelineObject::MakeCopy(const GraphicPipelineObjectDesc& newConfig)
	{
		if (m_pPSO == nullptr) {
			CONSOLE_ERROR_MESSAGE("PipelineObject copy failed: m_pPSO is null (not initialized)");
			return nullptr;
		}

		if (m_xType == PipelineType::Compute) {
			CONSOLE_ERROR_MESSAGE("PipelineObject copy failed: compute PSO type is not supported (only graphics PSO is allowed).");
			return nullptr;
		}


		std::unique_ptr<PipelineObject> copy = std::make_unique<PipelineObject>(m_pDevice);
		copy->m_xType = this->m_xType;
		copy->m_xConfig = newConfig;
		copy->m_mCompiledShaders = this->m_mCompiledShaders;
		if (m_bExternalRootSig) copy->SetExternalRootSignature(m_pRootSignature.Get());
			
		copy->CreatePSO();

		return copy;
	}

	wrl::ComPtr<IDxcBlob> PipelineObject::CompileShader(const ShaderDesc& desc)
	{
		std::vector<char> data;
		if (desc.isFile) {

			std::ifstream file(desc.pathOrData, std::ios::binary | std::ios::ate);
			if (!file) throw std::runtime_error("Can't open shader file");

			data.resize(file.tellg());
			file.seekg(0);
			file.read(data.data(), data.size());
		}
		else {
			std::string str = WStringToString(desc.pathOrData);
			data = std::vector<char>(str.begin(), str.end());
			data.push_back('\0');
		}

		
		return CompileShader(data, desc.entry, desc.target);
	}

	wrl::ComPtr<IDxcBlob> PipelineObject::CompileShader(std::vector<char> data, std::wstring entry, std::wstring target)
	{
		DxcBuffer buffer{ data.data(), data.size(), DXC_CP_UTF8 };

		std::vector<LPCWSTR> args = {
			L"-E", entry.c_str(),
			L"-T", target.c_str(),
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
		SAFE_ASSERT(desc.isFile, "Only file-based root signature loading is supported.");

		std::ifstream file(desc.pathOrData, std::ios::binary | std::ios::ate);
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


