#pragma once
#include "BasePipelineObject.h"


namespace FD3DW {

	struct HitGroupConfig
	{
		std::wstring ExportName;
		std::wstring ClosestHitShader;
		std::wstring AnyHitShader;
		std::wstring IntersectionShader;
		D3D12_HIT_GROUP_TYPE Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
	};

	struct RayTracingPipelineConfig
	{
		UINT MaxPayloadSize = 32;
		UINT MaxAttributeSize = 8;
		UINT MaxRecursionDepth = 1;

		std::vector<HitGroupConfig> HitGroups;
		std::unordered_map<std::wstring, Microsoft::WRL::ComPtr<ID3D12RootSignature>> LocalRootSignatures;
	};

	class RTPipelineObject : public BasePipelineObject {
	public:
		RTPipelineObject(ID3D12Device5* device);
		virtual ~RTPipelineObject() = default;

		virtual bool CreatePSOAfterCopy() override;
		bool CreatePSO(const std::unordered_map<CompileFileType, CompileDesc>& shaders) override;
		void Bind(ID3D12GraphicsCommandList* cmdList) override;
		std::unique_ptr<BasePipelineObject> MakeCopy() const override;

		void SetConfig(const RayTracingPipelineConfig& config);
		RayTracingPipelineConfig GetConfig() const;

		ID3D12StateObject* GetStateObject();
		const std::wstring& GetRayGenEntry() const;
		const std::vector<std::wstring>& GetMissEntries() const;
		const std::vector<std::wstring>& GetHitGroupEntries() const;

	private:
		void ExtractLocalRootSignatures(const std::unordered_map<CompileFileType, CompileDesc>& shaders);
		
	private:
		ID3D12Device5* m_pDXRDevice = nullptr;
		ID3D12GraphicsCommandList5* m_pPrevCmdList = nullptr;;
		RayTracingPipelineConfig m_xConfig;
		wrl::ComPtr<ID3D12StateObject> m_pPSO;

		std::wstring m_sRayGenEntry;
		std::vector<std::wstring> m_vMissEntries;
		std::vector<std::wstring> m_vHitGroupEntries;
	};

}
