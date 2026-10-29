#include <pch.h>

#include <Config/LightingConfig.h>
#include <Config/PipelineRegistry.h>
#include <Config/RendererLimits.h>
#include <Config/RendererResources.h>
#include <Config/RenderFormats.h>
#include <Config/ShadowConfig.h>
#include <D3DFramework/GraphicUtilites/StructuredBuffer.h>

static const GBuffersData s_xGBuffersData = { {
	DXGI_FORMAT_R16G16B16A16_FLOAT
} };
static std::unique_ptr<FD3DW::StructuredBuffer> s_pEmptyStructuredBuffer;
static std::map<ID3D12Device*, std::pair<UINT, UINT>> s_mDescriptorSizesForDevice;

const GBuffersData& GetGBufferData() {
	return s_xGBuffersData;
}

UINT GetGBuffersNum() {
	return static_cast<UINT>(s_xGBuffersData.GBuffersFormats.size());
}

void CreateEmptyStructuredBuffer(ID3D12Device* device) {
	s_pEmptyStructuredBuffer = FD3DW::StructuredBuffer::CreateStructuredBuffer<std::uint8_t>(device, 1u, false);
}

D3D12_GPU_VIRTUAL_ADDRESS GetEmptyStructuredBufferGPUVirtualAddress() {
	return s_pEmptyStructuredBuffer->GetResource()->GetGPUVirtualAddress();
}

const std::unordered_map<PSOType, PSODescriptor>& GetGraphicsPSODescriptors() {
	CD3DX12_RASTERIZER_DESC rasterizer(D3D12_DEFAULT);
	rasterizer.FillMode = D3D12_FILL_MODE_SOLID;
	rasterizer.CullMode = D3D12_CULL_MODE_NONE;

	CD3DX12_DEPTH_STENCIL_DESC depthStencil;
	depthStencil.DepthEnable = true;
	depthStencil.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	depthStencil.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	depthStencil.StencilEnable = false;
	depthStencil.StencilReadMask = 0xFF;
	depthStencil.StencilWriteMask = 0xFF;
	depthStencil.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	depthStencil.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	depthStencil.FrontFace.StencilPassOp = D3D12_STENCIL_OP_INCR_SAT;
	depthStencil.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	depthStencil.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	depthStencil.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	depthStencil.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	depthStencil.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;

	static const std::unordered_map<PSOType, PSODescriptor> s_mDescriptors = {
		{
			PSOType::DefferedFirstPassDefaultConfig,
			{
				PSOType::None,
				L"DefferedFirstPass",
				{},
				[rasterizer, depthStencil] {
					FD3DW::GraphicPipelineObjectDesc descriptor{};
					descriptor.NumRenderTargets = static_cast<UINT>(s_xGBuffersData.GBuffersFormats.size());

					for (UINT index = 0; index < descriptor.NumRenderTargets; ++index) {
						descriptor.RTVFormats[index] = s_xGBuffersData.GBuffersFormats[index];
					}

					descriptor.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
					descriptor.DepthStencilState = depthStencil;
					descriptor.RasterizerState = rasterizer;
					descriptor.TopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

					return descriptor;
				}()
			}
		}
	};

	return s_mDescriptors;
}

const std::unordered_map<PSOType, PSORTDescriptor>& GetRTPSODescriptors() {
	static const std::unordered_map<PSOType, PSORTDescriptor> s_mDescriptors = {
		{
			PSOType::AtlasRTShadowDefaultConfig,
			{
				PSOType::None,
				L"AtlasRTShadow",
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
		},
		{
			PSOType::NeuralShadowRTTarget,
			{
				PSOType::None,
				L"NeuralShadowRTTarget",
				{},
				[] {
					FD3DW::RayTracingPipelineConfig config;
					config.MaxPayloadSize = sizeof(float);
					config.MaxAttributeSize = sizeof(dx::XMFLOAT2);
					config.MaxRecursionDepth = 1;
					config.HitGroups.push_back({
						L"MyHitGroup",
						L"ClosestHit",
						L"AnyHit",
						L"",
						D3D12_HIT_GROUP_TYPE_TRIANGLES
					});

					return config;
				}()
			}
		}
	};

	return s_mDescriptors;
}

const std::unordered_map<PSOType, PSOComputeDescriptor>& GetComputePSODescriptors() {
	static const std::unordered_map<PSOType, PSOComputeDescriptor> s_mDescriptors = {
		{ PSOType::ClusteredShading_LightsToClusteresPass, { PSOType::None, L"ClusteredShading_LightsToClusteresPass", {} } },
		{ PSOType::ClusteredShading_BuildGridPass, { PSOType::None, L"ClusteredShading_BuildGridPass", {} } },
		{ PSOType::NeuralShadowWorkGeneration, { PSOType::None, L"NeuralShadowWorkGeneration", {} } },
		{ PSOType::NeuralShadowBaseline, { PSOType::None, L"NeuralShadowBaseline", {} } },
		{ PSOType::NeuralShadowInputCapture, { PSOType::None, L"NeuralShadowInputCapture", {}, true } }
	};

	return s_mDescriptors;
}

const std::map<std::wstring, std::tuple<FD3DW::CompileFileType, std::wstring, std::wstring>>& GetKnownShadersData() {
	static const std::map<std::wstring, std::tuple<FD3DW::CompileFileType, std::wstring, std::wstring>> s_mKnownShaders = {
		{ L"VS.hlsl", { FD3DW::CompileFileType::VS, L"VS", L"vs_6_5" } },
		{ L"PS.hlsl", { FD3DW::CompileFileType::PS, L"PS", L"ps_6_5" } },
		{ L"CS.hlsl", { FD3DW::CompileFileType::CS, L"CS", L"cs_6_5" } },
		{ L"DS.hlsl", { FD3DW::CompileFileType::DS, L"DS", L"ds_6_5" } },
		{ L"HS.hlsl", { FD3DW::CompileFileType::HS, L"HS", L"hs_6_5" } },
		{ L"GS.hlsl", { FD3DW::CompileFileType::GS, L"GS", L"gs_6_5" } },
		{ L"RayGen.hlsl", { FD3DW::CompileFileType::RayGen, L"RayGen", L"lib_6_5" } },
		{ L"ClosestHit.hlsl", { FD3DW::CompileFileType::ClosestHit, L"ClosestHit", L"lib_6_5" } },
		{ L"AnyHit.hlsl", { FD3DW::CompileFileType::AnyHit, L"AnyHit", L"lib_6_5" } },
		{ L"Miss.hlsl", { FD3DW::CompileFileType::Miss, L"Miss", L"lib_6_5" } },
		{ L"RS.hlsl", { FD3DW::CompileFileType::RootSignature, L"RS", L"rootsig_1_1" } }
	};

	return s_mKnownShaders;
}

void InitializeDescriptorSizes(ID3D12Device* device, UINT rtvDescSize, UINT cbvSrvUavDescSize) {
	s_mDescriptorSizesForDevice[device] = { rtvDescSize, cbvSrvUavDescSize };
}

const UINT& GetRTVDescriptorSize(ID3D12Device* device) {
	return s_mDescriptorSizesForDevice.at(device).first;
}

const UINT& GetCBV_SRV_UAVDescriptorSize(ID3D12Device* device) {
	return s_mDescriptorSizesForDevice.at(device).second;
}
