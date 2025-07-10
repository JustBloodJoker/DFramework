#include "GlobalConfig.h"

const std::unordered_map<PSOType, PSODescriptor>& GetPSODescriptors() {

    static CD3DX12_RASTERIZER_DESC rasterizerDesc(D3D12_DEFAULT);
    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
    rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;

    static const std::unordered_map<PSOType, PSODescriptor> descriptors = {
        {
            PSOType::DefferedFirstPassAnimatedMeshesDefaultConfig,
            {
                L"DefferedFirstPassAnimatedMeshes",
                [] {
                    FD3DW::GraphicPipelineObjectDesc desc{};
                    desc.NumRenderTargets = 1;
                    desc.RTVFormats[0] = DXGI_FORMAT_R32G32B32A32_FLOAT;
                    desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
                    desc.RasterizerState = rasterizerDesc;
                    return desc;
                }()
            }
        },
        {
            PSOType::DefferedFirstPassSimpleMeshesDefaultConfig,
            {
                L"DefferedFirstPassSimpleMeshes",
                [] {
                    FD3DW::GraphicPipelineObjectDesc desc{};
                    desc.NumRenderTargets = 1;
                    desc.RTVFormats[0] = DXGI_FORMAT_R32G32B32A32_FLOAT;
                    desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
                    desc.RasterizerState = rasterizerDesc;
                    return desc;
                }()
            }
        },
        {
            PSOType::DefferedSecondPassDefaultConfig,
            {
                L"DefferedSecondPass",
                [] {
                    FD3DW::GraphicPipelineObjectDesc desc{};
                    desc.NumRenderTargets = 1;
                    desc.RTVFormats[0] = DEFAULT_SWAPCHAIN_RTV_TYPE;
                    desc.DSVFormat = DXGI_FORMAT_UNKNOWN;
                    desc.RasterizerState = rasterizerDesc;
                    return desc;
                }()
            }
        }
    };

    return descriptors;
}

const std::map<std::wstring, std::tuple<FD3DW::CompileFileType, std::wstring, std::wstring>>& GetKnownShadersData()
{
    static const std::map<std::wstring, std::tuple<FD3DW::CompileFileType, std::wstring, std::wstring>> s_mKnownShaders = {
        {L"VS.hlsl", {FD3DW::CompileFileType::VS, L"VS", L"vs_6_5"}},
        {L"PS.hlsl", {FD3DW::CompileFileType::PS, L"PS", L"ps_6_5"}},
        {L"CS.hlsl", {FD3DW::CompileFileType::CS, L"CS", L"cs_6_5"}},
        {L"DS.hlsl", {FD3DW::CompileFileType::DS, L"DS", L"ds_6_5"}},
        {L"HS.hlsl", {FD3DW::CompileFileType::HS, L"HS", L"hs_6_5"}},
        {L"GS.hlsl", {FD3DW::CompileFileType::GS, L"GS", L"gs_6_5"}},
        {L"RS.hlsl", {FD3DW::CompileFileType::RootSignature, L"RS", L"rootsig_1_1"}}
    };

    return s_mKnownShaders;
}




