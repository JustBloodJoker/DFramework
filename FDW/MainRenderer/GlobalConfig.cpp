#include "GlobalConfig.h"
#include <D3DFramework/GraphicUtilites/StructuredBuffer.h>

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

const DXGI_FORMAT GetForwardRenderPassFormat() {
    return DXGI_FORMAT_R32G32B32A32_FLOAT;
}

UINT GetGBuffersNum() {
    return (UINT)s_sGBuffersData.GBuffersFormats.size();
}

std::unique_ptr<FD3DW::StructuredBuffer> s_sEmptyStructuredBuffer;

void CreateEmptyStructuredBuffer(ID3D12Device* device) {
    s_sEmptyStructuredBuffer = FD3DW::StructuredBuffer::CreateStructuredBuffer<uint8_t>(device, 1, false);
}

D3D12_GPU_VIRTUAL_ADDRESS GetEmptyStructuredBufferGPUVirtualAddress() {
    return s_sEmptyStructuredBuffer->GetResource()->GetGPUVirtualAddress();
}

const std::unordered_map<PSOType, PSODescriptor>& GetGraphicsPSODescriptors() {

    CD3DX12_RASTERIZER_DESC rasterizerDesc(D3D12_DEFAULT);
    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
    rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;


    CD3DX12_DEPTH_STENCIL_DESC dsvPostProcessDesc(D3D12_DEFAULT);
    dsvPostProcessDesc.DepthEnable = FALSE;
    dsvPostProcessDesc.StencilEnable = FALSE;


    CD3DX12_DEPTH_STENCIL_DESC dsvFirstDefPassDesc;
    dsvFirstDefPassDesc.DepthEnable = true;
    dsvFirstDefPassDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    dsvFirstDefPassDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    dsvFirstDefPassDesc.StencilEnable = false;
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

    
    CD3DX12_DEPTH_STENCIL_DESC skyboxDepthDesc(D3D12_DEFAULT);
    skyboxDepthDesc.DepthEnable = true;
    skyboxDepthDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    skyboxDepthDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

    CD3DX12_DEPTH_STENCIL_DESC secondPassDSVDesc(D3D12_DEFAULT);
    secondPassDSVDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
   

    CD3DX12_RASTERIZER_DESC skyboxRasterizerDesc(D3D12_DEFAULT);
    skyboxRasterizerDesc.CullMode = D3D12_CULL_MODE_FRONT;

    static const std::unordered_map<PSOType, PSODescriptor> descriptors = {
        {
            PSOType::DefferedFirstPassDefaultConfig,
            {
                PSOType::None,
                L"DefferedFirstPass",
                {},
                [rasterizerDesc, dsvFirstDefPassDesc] {
                    FD3DW::GraphicPipelineObjectDesc desc{};
                    desc.NumRenderTargets = UINT(s_sGBuffersData.GBuffersFormats.size());
                    for (UINT i = 0; i < desc.NumRenderTargets; ++i) {
                        desc.RTVFormats[i] = s_sGBuffersData.GBuffersFormats[i];
                    }
                    desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
                    desc.DepthStencilState = dsvFirstDefPassDesc;
                    desc.RasterizerState = rasterizerDesc;
                    desc.TopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
                    return desc;
                }()
            }
        },
        {
            PSOType::DefferedFirstPassWithPreDepth,
            {
                PSOType::DefferedFirstPassDefaultConfig,
                L"DefferedFirstPass",
                {},
                [rasterizerDesc] {
                    FD3DW::GraphicPipelineObjectDesc desc{};
                    desc.NumRenderTargets = UINT(s_sGBuffersData.GBuffersFormats.size());
                    for (UINT i = 0; i < desc.NumRenderTargets; ++i) {
                        desc.RTVFormats[i] = s_sGBuffersData.GBuffersFormats[i];
                    }
                    desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
                    CD3DX12_DEPTH_STENCIL_DESC dsvFirstPassWithPre(D3D12_DEFAULT);
                    dsvFirstPassWithPre.DepthEnable = true;
                    dsvFirstPassWithPre.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
                    dsvFirstPassWithPre.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
                    dsvFirstPassWithPre.StencilEnable = false;
                    desc.DepthStencilState = dsvFirstPassWithPre;
                    desc.RasterizerState = rasterizerDesc;
                    desc.TopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
                    return desc;
                }()
            }
        },
        {
            PSOType::PreDepthDefaultConfig,
            {
                PSOType::None,
                L"PreDepth",
                {},
                [rasterizerDesc, dsvFirstDefPassDesc] {
                    FD3DW::GraphicPipelineObjectDesc desc{};
                    desc.NumRenderTargets = 0;
                    desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
                    desc.DepthStencilState = dsvFirstDefPassDesc;
                    desc.RasterizerState = rasterizerDesc;
                    desc.TopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
                    return desc;
                }()
            }
        },
        {
            PSOType::DefferedSecondPassDefaultConfig,
            {
                PSOType::None,
                L"DefferedSecondPass",
                {},
                [rasterizerDesc, secondPassDSVDesc] {
                    FD3DW::GraphicPipelineObjectDesc desc{};
                    desc.NumRenderTargets = 1;
                    desc.RTVFormats[0] = GetForwardRenderPassFormat();
                    desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
                    desc.DepthStencilState = secondPassDSVDesc;
                    desc.RasterizerState = rasterizerDesc;
                    desc.TopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
                    return desc;
                }()
            }
        },
        {
            PSOType::SimpleSkyboxDefaultConfig,
            {
                PSOType::None,
                L"SimpleSkybox",
                {},
                [skyboxRasterizerDesc, skyboxDepthDesc] {
                    FD3DW::GraphicPipelineObjectDesc desc{};
                    desc.NumRenderTargets = 1;
                    desc.RTVFormats[0] = GetForwardRenderPassFormat();
                    desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
                    desc.RasterizerState = skyboxRasterizerDesc;
                    desc.DepthStencilState = skyboxDepthDesc;
                    desc.TopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
                    return desc;
                }()
            }
        },
        {
            PSOType::PostProcessDefaultConfig,
            {
                PSOType::None,
                L"PostProcess",
                {},
                [dsvPostProcessDesc, skyboxRasterizerDesc] {
                    FD3DW::GraphicPipelineObjectDesc desc{};
                    desc.NumRenderTargets = 1;
                    desc.RTVFormats[0] = DEFAULT_SWAPCHAIN_RTV_TYPE;
                    desc.DSVFormat = DXGI_FORMAT_UNKNOWN;
                    desc.RasterizerState = skyboxRasterizerDesc;
                    desc.DepthStencilState = dsvPostProcessDesc;
                    desc.TopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
                    return desc;
                }()
            }
        },
        {
            PSOType::BloomEffect_BrightPass,
            {
                PSOType::None,
                L"BrightPass",
                {},
                [dsvPostProcessDesc, rasterizerDesc] {
                    FD3DW::GraphicPipelineObjectDesc desc{};
                    desc.NumRenderTargets = 1;
                    desc.RTVFormats[0] = DEFAULT_SWAPCHAIN_RTV_TYPE;
                    desc.DSVFormat = DXGI_FORMAT_UNKNOWN;
                    desc.RasterizerState = rasterizerDesc;
                    desc.DepthStencilState = dsvPostProcessDesc;
                    desc.TopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
                    return desc;
                }()
            }
        },
        {
            PSOType::BloomEffect_GaussianBlurPass,
            {
                PSOType::None,
                L"GaussianBlur",
                {},
                [dsvPostProcessDesc, rasterizerDesc] {
                    FD3DW::GraphicPipelineObjectDesc desc{};
                    desc.NumRenderTargets = 1;
                    desc.RTVFormats[0] = DEFAULT_SWAPCHAIN_RTV_TYPE;
                    desc.DSVFormat = DXGI_FORMAT_UNKNOWN;
                    desc.RasterizerState = rasterizerDesc;
                    desc.DepthStencilState = dsvPostProcessDesc;
                    desc.TopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
                    return desc;
                }()
            }
        },
        {
            PSOType::BloomEffect_CompositePass,
            {
                PSOType::None,
                L"CompositeBloom",
                {},
                [dsvPostProcessDesc, rasterizerDesc] {
                    FD3DW::GraphicPipelineObjectDesc desc{};
                    desc.NumRenderTargets = 1;
                    desc.RTVFormats[0] = DEFAULT_SWAPCHAIN_RTV_TYPE;
                    desc.DSVFormat = DXGI_FORMAT_UNKNOWN;
                    desc.RasterizerState = rasterizerDesc;
                    desc.DepthStencilState = dsvPostProcessDesc;
                    desc.TopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
                    return desc;
                }()
            }
        }
    };

    return descriptors;
}

const std::unordered_map<PSOType, PSORTDescriptor>& GetRTPSODescriptors() {
    static const std::unordered_map<PSOType, PSORTDescriptor> descriptors = {
        {
            PSOType::RTSoftShadowDefaultConfig,
            {
                PSOType::None,
                L"RTSoftShadow",
                {},
                [] {
                    FD3DW::RayTracingPipelineConfig config;
                    config.MaxPayloadSize = sizeof(dx::XMFLOAT4);
                    config.MaxAttributeSize = sizeof(dx::XMFLOAT2);
                    config.MaxRecursionDepth = 1;

                    config.HitGroups.push_back({
                        L"MyHitGroup",
                        L"ClosestHit",
                        L"",
                        L"",
                        D3D12_HIT_GROUP_TYPE_TRIANGLES
                    });
                    return config;
                }()
            }
        }
    };

    return descriptors;
}

const std::unordered_map<PSOType, PSOComputeDescriptor>& GetComputePSODescriptors() {
    static const std::unordered_map<PSOType, PSOComputeDescriptor> descriptors = {
        {
            PSOType::ObjectsCullingDefaultConfig,
            {
                PSOType::None,
                L"ObjectsCulling",
                {},
            }
        },
        {
            PSOType::CopyDepthToHIZ,
            {
                PSOType::None,
                L"CopyDepthToHIZ",
                {},
            }
        },
        {
            PSOType::ClusteredShading_LightsToClusteresPass,
            {
                PSOType::None,
                L"ClusteredShading_LightsToClusteresPass",
                { }
            }
        },
        {
            PSOType::ClusteredShading_BuildGridPass,
            {
                PSOType::None,
                L"ClusteredShading_BuildGridPass",
                { }
            }
        }
    };
    return descriptors;
}

const std::map<std::wstring, std::tuple<FD3DW::CompileFileType, std::wstring, std::wstring>>& GetKnownShadersData()
{
    static const std::map<std::wstring, std::tuple<FD3DW::CompileFileType, std::wstring, std::wstring>> s_mKnownShaders = {
        {L"VS.hlsl",         {FD3DW::CompileFileType::VS,               L"VS",              L"vs_6_5"}},
        {L"PS.hlsl",         {FD3DW::CompileFileType::PS,               L"PS",              L"ps_6_5"}},
        {L"CS.hlsl",         {FD3DW::CompileFileType::CS,               L"CS",              L"cs_6_5"}},
        {L"DS.hlsl",         {FD3DW::CompileFileType::DS,               L"DS",              L"ds_6_5"}},
        {L"HS.hlsl",         {FD3DW::CompileFileType::HS,               L"HS",              L"hs_6_5"}},
        {L"GS.hlsl",         {FD3DW::CompileFileType::GS,               L"GS",              L"gs_6_5"}},		
        {L"RayGen.hlsl",     {FD3DW::CompileFileType::RayGen,           L"RayGen",          L"lib_6_5"}},
        {L"ClosestHit.hlsl", {FD3DW::CompileFileType::ClosestHit,       L"ClosestHit",      L"lib_6_5"}},		
        {L"Miss.hlsl",       {FD3DW::CompileFileType::Miss,             L"Miss",            L"lib_6_5"}},
        {L"RS.hlsl",         {FD3DW::CompileFileType::RootSignature,    L"RS",              L"rootsig_1_1"}}
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




