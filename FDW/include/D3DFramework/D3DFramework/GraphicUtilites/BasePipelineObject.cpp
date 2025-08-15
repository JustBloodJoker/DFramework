#include "BasePipelineObject.h"


namespace FD3DW {

	static const std::vector<CompileFileType> s_vCompiledFilesForCheckRootSignature = {
		CompileFileType::RootSignature,
		CompileFileType::VS,
		CompileFileType::PS,
		CompileFileType::CS,
		CompileFileType::RayGen
	};

	BasePipelineObject::BasePipelineObject(ID3D12Device* device)
	{
		Initialize(device);
	}


	bool BasePipelineObject::Initialize(ID3D12Device* device) {
		m_pDevice = device;
		DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(m_pUtils.ReleaseAndGetAddressOf()));
		DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(m_pCompiler.ReleaseAndGetAddressOf()));
		m_pUtils->CreateDefaultIncludeHandler(m_pIncludeHandler.ReleaseAndGetAddressOf());
		return true;
	}

	void BasePipelineObject::SetIncludeDirectories(const std::vector<std::wstring>& includeDirs) {
		m_vIncludeDirs = includeDirs;
	}

	void BasePipelineObject::SetExternalRootSignature(ID3D12RootSignature* externalRootSig) {
		m_pRootSignature = externalRootSig;
		m_bExternalRootSig = true;
	}

	ID3D12RootSignature* BasePipelineObject::GetRootSignature()
	{
		return m_pRootSignature.Get();
	}

	wrl::ComPtr<IDxcBlob> BasePipelineObject::CompileShader(const CompileDesc& desc) {
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
	wrl::ComPtr<IDxcBlob> BasePipelineObject::CompileShader(std::vector<char> data, std::wstring entry, std::wstring target) {
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
			L"-Ges",
			L"-fspv-reflect",
		};

		for (const auto& dir : m_vIncludeDirs) {
			args.push_back(L"-I");
			args.push_back(dir.c_str());
		}

		wrl::ComPtr<IDxcOperationResult> result;
		m_pCompiler->Compile(&buffer, args.data(), (UINT)args.size(), m_pIncludeHandler.Get(), IID_PPV_ARGS(result.ReleaseAndGetAddressOf()));

		HRESULT_ASSERT(result->GetStatus(&hr), "Failed to retrieve shader compile status");

		wrl::ComPtr<IDxcBlobEncoding> errors;
		result->GetErrorBuffer(&errors);
		if (errors && errors->GetBufferSize()) {
			CONSOLE_ERROR_MESSAGE((char*)errors->GetBufferPointer());
			return nullptr;
		}

		wrl::ComPtr<IDxcBlob> shaderBlob;
		result->GetResult(&shaderBlob);
		return shaderBlob;
	}

	wrl::ComPtr<IDxcBlob> BasePipelineObject::CompileRootSignatureBlob(const CompileDesc& desc) {
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
			L"-Zi",
			L"-Ges",
			L"-fspv-reflect",
		};

		wrl::ComPtr<IDxcOperationResult> result;
		HRESULT_ASSERT(m_pCompiler->Compile(&buffer, args.data(), (UINT)args.size(), m_pIncludeHandler.Get(), IID_PPV_ARGS(result.ReleaseAndGetAddressOf())), "Shader compile error");

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

	wrl::ComPtr<ID3D12RootSignature> BasePipelineObject::ExtractRootSignature(IDxcBlob* blob) {
		wrl::ComPtr<ID3D12RootSignature> rs;
		hr = m_pDevice->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(rs.ReleaseAndGetAddressOf()));
		return rs;
	}

	void BasePipelineObject::CompileShaders(const std::unordered_map<CompileFileType, CompileDesc>& shaders)
	{
		for (const auto& [stage, desc] : shaders) {
			if (stage == CompileFileType::RootSignature) {
				m_mCompiledShaders[stage] = CompileRootSignatureBlob(desc);
			}
			else {
				m_mCompiledShaders[stage] = CompileShader(desc);
			}

			SAFE_ASSERT(m_mCompiledShaders[stage], "Shader compilation failed");
		}
	}

	void BasePipelineObject::ExtractRootSignature()
	{
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
	}

	bool BasePipelineObject::HasGraphicsStages(const std::unordered_map<CompileFileType, CompileDesc>& shaders) const {
		for (CompileFileType stage : { CompileFileType::VS }) {
			if (shaders.count(stage)) return true;
		}
		return false;
	}
	bool BasePipelineObject::HasComputeStage(const std::unordered_map<CompileFileType, CompileDesc>& shaders) const {
		return shaders.count(CompileFileType::CS) > 0;
	}
}