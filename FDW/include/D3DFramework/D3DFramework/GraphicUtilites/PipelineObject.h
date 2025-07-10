#pragma once
#include "../pch.h"

namespace FD3DW {
    
    enum class CompileFileType {
        VS, 
        PS, 
        GS, 
        HS, 
        DS,
        CS,
        RootSignature
    };

    struct CompileDesc {
        std::wstring pathOrData;
        std::wstring entry;
        std::wstring target;
        bool isFile = true;
    };
    
    enum class PipelineType {
        Graphics,
        Compute
    };

    struct GraphicPipelineObjectDesc {
        CD3DX12_BLEND_DESC BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        CD3DX12_RASTERIZER_DESC RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        CD3DX12_DEPTH_STENCIL_DESC DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        D3D12_PRIMITIVE_TOPOLOGY_TYPE TopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        DXGI_FORMAT RTVFormats[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
        UINT NumRenderTargets = 0;
        DXGI_FORMAT DSVFormat = DXGI_FORMAT_UNKNOWN;
        DXGI_SAMPLE_DESC SampleDesc = { 1, 0 };
        UINT SampleMask = UINT_MAX;
        UINT NodeMask = 0;
    };

	class PipelineObject {
    public:
        PipelineObject(ID3D12Device* device);
        virtual ~PipelineObject() = default;

        bool Initialize(ID3D12Device* device);

        void SetIncludeDirectories(const std::vector<std::wstring>& includeDirs);
        void SetExternalRootSignature(ID3D12RootSignature* external); 
     
        bool CreatePSO(const std::unordered_map<CompileFileType, CompileDesc>& shaders, const GraphicPipelineObjectDesc& config = {});

        void Bind(ID3D12GraphicsCommandList* cmdList);
        
        ID3D12PipelineState* GetPSO() const;
        ID3D12RootSignature* GetRootSignature() const;
        GraphicPipelineObjectDesc GetGraphicPSOConfig() const;



    public:
        std::unique_ptr<PipelineObject> MakeCopy(const GraphicPipelineObjectDesc& config);

    private:
        bool CreatePSO();

        std::unordered_map<CompileFileType, wrl::ComPtr<IDxcBlob>> m_mCompiledShaders;
        GraphicPipelineObjectDesc m_xConfig;


    private:
        ID3D12Device* m_pDevice = nullptr;
        PipelineType m_xType = PipelineType::Graphics;

        wrl::ComPtr<IDxcUtils> m_pUtils;
        wrl::ComPtr<IDxcCompiler3> m_pCompiler;
        wrl::ComPtr<IDxcIncludeHandler> m_pIncludeHandler;

        wrl::ComPtr<ID3D12RootSignature> m_pRootSignature;
        wrl::ComPtr<ID3D12PipelineState> m_pPSO;

        std::vector<D3D12_INPUT_ELEMENT_DESC> m_vInputLayout;
        std::vector<std::wstring> m_vIncludeDirs;
        bool m_bExternalRootSig = false;

    private:
        wrl::ComPtr<IDxcBlob> CompileShader(const CompileDesc& desc);
        wrl::ComPtr<IDxcBlob> CompileShader(std::vector<char> data, std::wstring entry, std::wstring target);
        wrl::ComPtr<IDxcBlob> CompileRootSignatureBlob(const CompileDesc& desc);
        wrl::ComPtr<ID3D12RootSignature> ExtractRootSignature(IDxcBlob* blob);
        std::vector<D3D12_INPUT_ELEMENT_DESC> ReflectInputLayout(IDxcBlob* blob);

        bool HasGraphicsStages(const std::unordered_map<CompileFileType, CompileDesc>& shaders) const;
        bool HasComputeStage(const std::unordered_map<CompileFileType, CompileDesc>& shaders) const;

	};


}