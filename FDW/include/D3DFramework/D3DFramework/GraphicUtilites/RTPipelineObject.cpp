#include "RTPipelineObject.h"


namespace FD3DW {

	RTPipelineObject::RTPipelineObject(ID3D12Device5* device) :BasePipelineObject(device) {
		m_pDXRDevice = device;
	}

	bool RTPipelineObject::CreatePSO(const std::unordered_map<CompileFileType, CompileDesc>& shaders)
	{
        CompileShaders(shaders);
        ExtractRootSignature();
        ExtractLocalRootSignatures(shaders);

        CD3DX12_STATE_OBJECT_DESC stateObjectDesc(D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE);

        std::unordered_map<std::wstring, std::wstring> compiledEntries;

        for (const auto& [type, desc] : shaders)
        {
            if (type == CompileFileType::RootSignature)
                continue;

            auto it = m_mCompiledShaders.find(type);
            if (it == m_mCompiledShaders.end())
                continue;

            const auto& blob = it->second;

            auto dxilLib = stateObjectDesc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
            D3D12_SHADER_BYTECODE shaderBytecode = {};
            shaderBytecode.pShaderBytecode = blob->GetBufferPointer();
            shaderBytecode.BytecodeLength = blob->GetBufferSize();

            dxilLib->SetDXILLibrary(&shaderBytecode);
            dxilLib->DefineExport(desc.entry.c_str());

            switch (type) {
            case CompileFileType::RayGen:
                m_sRayGenEntry = desc.entry;
                break;
            case CompileFileType::Miss:
                m_vMissEntries.push_back(desc.entry);
                break;
            default:
                break;
            }
        }

        for (const auto& hitGroupConfig : m_xConfig.HitGroups)
        {
            auto hitGroup = stateObjectDesc.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            hitGroup->SetHitGroupExport(hitGroupConfig.ExportName.c_str());
            hitGroup->SetHitGroupType(hitGroupConfig.Type);

            if (!hitGroupConfig.ClosestHitShader.empty())
                hitGroup->SetClosestHitShaderImport(hitGroupConfig.ClosestHitShader.c_str());

            if (!hitGroupConfig.AnyHitShader.empty())
                hitGroup->SetAnyHitShaderImport(hitGroupConfig.AnyHitShader.c_str());

            if (!hitGroupConfig.IntersectionShader.empty())
                hitGroup->SetIntersectionShaderImport(hitGroupConfig.IntersectionShader.c_str());


            m_vHitGroupEntries.push_back(hitGroupConfig.ExportName);
        }

        auto shaderConfig = stateObjectDesc.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
        shaderConfig->Config(m_xConfig.MaxPayloadSize, m_xConfig.MaxAttributeSize);

        if (m_pRootSignature && !m_bExternalRootSig)
        {
            auto globalRS = stateObjectDesc.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
            globalRS->SetRootSignature(m_pRootSignature.Get());
        }

        std::vector<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT*> associations;

        for (const auto& [entry, rootSig] : m_xConfig.LocalRootSignatures)
        {
            auto localRS = stateObjectDesc.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
            localRS->SetRootSignature(rootSig.Get());

            auto assoc = stateObjectDesc.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
            assoc->SetSubobjectToAssociate(*localRS);
            assoc->AddExport(entry.c_str());

            associations.push_back(assoc);
        }

        auto pipelineConfig = stateObjectDesc.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
        pipelineConfig->Config(m_xConfig.MaxRecursionDepth);

        hr = m_pDXRDevice->CreateStateObject(stateObjectDesc, IID_PPV_ARGS(m_pPSO.ReleaseAndGetAddressOf()));
        if (FAILED(hr))
        {
            OutputDebugString(L"[DXR] Failed to create state object (PSO).\n");
            return false;
        }

        return true;
	}

	void RTPipelineObject::Bind(ID3D12GraphicsCommandList* cmdList)
	{
        if (m_pPrevCmdList != cmdList) {
            hr = cmdList->QueryInterface(IID_PPV_ARGS(&m_pPrevCmdList));
        }
        
        m_pPrevCmdList->SetPipelineState1(m_pPSO.Get());
        m_pPrevCmdList->SetComputeRootSignature(m_pRootSignature.Get());
	}

	std::unique_ptr<BasePipelineObject> RTPipelineObject::MakeCopy() const
	{
		return nullptr;
	}

	void RTPipelineObject::SetConfig(const RayTracingPipelineConfig& config) {
		m_xConfig = config;
	}

	RayTracingPipelineConfig RTPipelineObject::GetConfig() const {
		return m_xConfig;
	}

    ID3D12StateObject* RTPipelineObject::GetStateObject()
    {
        return m_pPSO.Get();
    }

    const std::wstring& RTPipelineObject::GetRayGenEntry() const
    {
        return m_sRayGenEntry;
    }

    const std::vector<std::wstring>& RTPipelineObject::GetMissEntries() const
    {
        return m_vMissEntries;
    }

    const std::vector<std::wstring>& RTPipelineObject::GetHitGroupEntries() const
    {
        return m_vHitGroupEntries;
    }

    void RTPipelineObject::ExtractLocalRootSignatures(const std::unordered_map<CompileFileType, CompileDesc>& shaders)
    {
        for (const auto& [type, desc] : shaders)
        {
            if (type == CompileFileType::RootSignature) continue;

            auto it = m_mCompiledShaders.find(type);
            if (it == m_mCompiledShaders.end())
                continue;

            if(auto rs = ExtractRootSignature(m_mCompiledShaders[type].Get())) m_xConfig.LocalRootSignatures[desc.entry] = rs;
        }
    }


}
