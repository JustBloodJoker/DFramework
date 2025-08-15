#pragma once
#include "../pch.h"
#include <unordered_map>
#include <string>
#include <vector>
#include <wrl.h>
#include <dxcapi.h>

namespace FD3DW {

	enum class CompileFileType {
		VS, PS, GS, HS, DS, CS, RootSignature, RayGen, Miss, ClosestHit
	};

	struct CompileDesc {
		std::wstring pathOrData;
		std::wstring entry;
		std::wstring target;
		bool isFile = true;
	};

	class BasePipelineObject {
	public:
		BasePipelineObject(ID3D12Device* device);
		virtual ~BasePipelineObject() = default;

		bool Initialize(ID3D12Device* device);

		void SetIncludeDirectories(const std::vector<std::wstring>& includeDirs);
		void SetExternalRootSignature(ID3D12RootSignature* externalRootSig);

		virtual bool CreatePSO(const std::unordered_map<CompileFileType, CompileDesc>& shaders) = 0;
		virtual void Bind(ID3D12GraphicsCommandList* cmdList) = 0;
		virtual std::unique_ptr<BasePipelineObject> MakeCopy() const = 0;

		//for RT PO returning global root signature
		ID3D12RootSignature* GetRootSignature();

	protected:
		wrl::ComPtr<IDxcBlob> CompileShader(const CompileDesc& desc);
		wrl::ComPtr<IDxcBlob> CompileShader(std::vector<char> data, std::wstring entry, std::wstring target);
		wrl::ComPtr<IDxcBlob> CompileRootSignatureBlob(const CompileDesc& desc);
		wrl::ComPtr<ID3D12RootSignature> ExtractRootSignature(IDxcBlob* blob);

		void CompileShaders(const std::unordered_map<CompileFileType, CompileDesc>& shaders);
		void ExtractRootSignature();

		bool HasGraphicsStages(const std::unordered_map<CompileFileType, CompileDesc>& shaders) const;
		bool HasComputeStage(const std::unordered_map<CompileFileType, CompileDesc>& shaders) const;

	protected:
		ID3D12Device* m_pDevice = nullptr;
		bool m_bExternalRootSig = false;

		std::vector<std::wstring> m_vIncludeDirs;
		std::unordered_map<CompileFileType, wrl::ComPtr<IDxcBlob>> m_mCompiledShaders;

		wrl::ComPtr<ID3D12RootSignature> m_pRootSignature;

		wrl::ComPtr<IDxcUtils> m_pUtils;
		wrl::ComPtr<IDxcCompiler3> m_pCompiler;
		wrl::ComPtr<IDxcIncludeHandler> m_pIncludeHandler;
	};

}
