#include "GlobalConfig.h"

static const GBuffersData s_sGBuffersData = { {
        DXGI_FORMAT_R32G32B32A32_FLOAT,
        DXGI_FORMAT_R32G32B32A32_FLOAT,
        DXGI_FORMAT_R32G32B32A32_FLOAT,
        DXGI_FORMAT_R32G32B32A32_FLOAT,
        DXGI_FORMAT_R32G32B32A32_FLOAT,
        DXGI_FORMAT_R32G32B32A32_FLOAT
}};



const GBuffersData& GetGBufferData() {
    return s_sGBuffersData;
}

const UINT& GetGBuffersNum() {
    return (UINT)s_sGBuffersData.GBuffersFormats.size();
}

const std::unordered_map<PSOType, PSODescriptor>& GetPSODescriptors() {

    static CD3DX12_RASTERIZER_DESC rasterizerDesc(D3D12_DEFAULT);
    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
    rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;

    static CD3DX12_DEPTH_STENCIL_DESC dsvFirstDefPassDesc;
    dsvFirstDefPassDesc.DepthEnable = true;
    dsvFirstDefPassDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    dsvFirstDefPassDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    dsvFirstDefPassDesc.StencilEnable = true;
    dsvFirstDefPassDesc.StencilReadMask = 0xFF;
    dsvFirstDefPassDesc.StencilWriteMask = 0xFF;
    dsvFirstDefPassDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
    dsvFirstDefPassDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
    dsvFirstDefPassDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_INCR_SAT;
    dsvFirstDefPassDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    dsvFirstDefPassDesc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
    dsvFirstDefPassDesc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
    dsvFirstDefPassDesc.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
    dsvFirstDefPassDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;

    
    static CD3DX12_DEPTH_STENCIL_DESC skyboxDepthDesc(D3D12_DEFAULT);
    skyboxDepthDesc.DepthEnable = true;
    skyboxDepthDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    skyboxDepthDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

    static CD3DX12_DEPTH_STENCIL_DESC secondPassDSVDesc(D3D12_DEFAULT);
    secondPassDSVDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    //secondPassDSVDesc.


    static CD3DX12_RASTERIZER_DESC skyboxRasterizerDesc(D3D12_DEFAULT);
    skyboxRasterizerDesc.CullMode = D3D12_CULL_MODE_FRONT;

    static const std::unordered_map<PSOType, PSODescriptor> descriptors = {
        {
            PSOType::DefferedFirstPassDefaultConfig,
            {
                L"DefferedFirstPass",
                [] {
                    FD3DW::GraphicPipelineObjectDesc desc{};
                    desc.NumRenderTargets = UINT(s_sGBuffersData.GBuffersFormats.size());
                    for (auto i = 0; i < desc.NumRenderTargets; ++i) {
                        desc.RTVFormats[i] = s_sGBuffersData.GBuffersFormats[i];
                    }
                    desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
                    desc.DepthStencilState = dsvFirstDefPassDesc;
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
                    desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
                    desc.DepthStencilState = secondPassDSVDesc;
                    desc.RasterizerState = rasterizerDesc;
                    return desc;
                }()
            }
        },
        {
            PSOType::SimpleSkyboxDefaultConfig,
            {
                L"SimpleSkybox",
                [] {
                    FD3DW::GraphicPipelineObjectDesc desc{};
                    desc.NumRenderTargets = 1;
                    desc.RTVFormats[0] = DEFAULT_SWAPCHAIN_RTV_TYPE;
                    desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
                    desc.RasterizerState = skyboxRasterizerDesc;
                    desc.DepthStencilState = skyboxDepthDesc;
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


/////////////////////////////////////////

static std::map<ID3D12Device*, std::tuple<UINT, UINT, UINT>> s_mDescSizesForDevice;

void InitializeDescriptorSizes(ID3D12Device* device, UINT rtvDescSize, UINT dsvDescSize, UINT cbv_srv_uavDescSize){
    s_mDescSizesForDevice[device] = { rtvDescSize, dsvDescSize, cbv_srv_uavDescSize };
}

const UINT& GetRTVDescriptorSize(ID3D12Device* device) {
    return std::get<0>(s_mDescSizesForDevice.at(device));
}

const UINT& GetDSVDescriptorSize(ID3D12Device* device) {
    return std::get<1>(s_mDescSizesForDevice.at(device));
}

const UINT& GetCBV_SRV_UAVDescriptorSize(ID3D12Device* device) {
    return std::get<2>(s_mDescSizesForDevice.at(device));
}




