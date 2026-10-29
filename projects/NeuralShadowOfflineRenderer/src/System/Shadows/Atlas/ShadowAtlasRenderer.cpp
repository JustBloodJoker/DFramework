#include "pch.h"

#include <System/Shadows/Atlas/ShadowAtlasRenderer.h>
#include <ShadowData/ShadowLightParameters.h>
#include <Renderer/PSOManager.h>
#include <Component/Light/LightComponentData.h>
#include <Config/RenderFormats.h>
#include <Config/ShaderBindings.h>
#include <Config/ShadowConfig.h>

ShadowAtlasRenderer::ShadowAtlasRenderer(IShadowRenderServices& services) : m_pServices(&services) { }

dx::XMFLOAT3 ShadowAtlasRenderer::Subtract(const dx::XMFLOAT3& left, const dx::XMFLOAT3& right) {
    return dx::XMFLOAT3(left.x - right.x, left.y - right.y, left.z - right.z);
}

void ShadowAtlasRenderer::Initialize() {
	auto device = m_pServices->GetDevice();
	auto dxrDevice = m_pServices->GetDXRDevice();

    auto pso = PSOManager::GetInstance()->GetPSOObjectAs<FD3DW::RTPipelineObject>(PSOType::AtlasRTShadowDefaultConfig);
    m_pHardShadowBindingTable = std::make_unique<FD3DW::RTShaderBindingTable>(pso);
    m_pHardShadowBindingTable->InitSBT(dxrDevice);

	m_pLightAtlasMetaBuffer = FD3DW::StructuredBuffer::CreateStructuredBuffer<LightAtlasMeta>(device, 1, true, D3D12_RESOURCE_FLAG_NONE, D3D12_HEAP_TYPE_DEFAULT);


    {
        std::lock_guard<std::mutex> lock(m_xHardBackend.m_xShadowParamsMutex);
        m_xHardBackend.m_xShadowParams.AtlasHeight = NSOfflineConfig::Shadows::AtlasHeight;
        m_xHardBackend.m_xShadowParams.AtlasWidth = NSOfflineConfig::Shadows::AtlasWidth;
        m_xHardBackend.m_xShadowParams.ScreenHeight = std::max(1, m_pServices->GetSceneRenderHeight());
        m_xHardBackend.m_xShadowParams.ScreenWidth = std::max(1, m_pServices->GetSceneRenderWidth());
    }

	m_pAtlasPerFrameDataBuffer = std::make_unique<FD3DW::UploadBuffer<AtlasRTShadowParams>>(device, 1, true);
    {
        std::lock_guard<std::mutex> lock(m_xHardBackend.m_xShadowParamsMutex);
	    m_pAtlasPerFrameDataBuffer->CpyData(0, m_xHardBackend.m_xShadowParams);
    }

    m_pTexelToLight = FD3DW::FResource::CreateAnonimTexture(device, 1, NSOfflineConfig::Shadows::AtlasLightIndexFormat, NSOfflineConfig::Shadows::AtlasWidth, NSOfflineConfig::Shadows::AtlasHeight, DXGI_SAMPLE_DESC({ 1,0 }), D3D12_RESOURCE_DIMENSION_TEXTURE2D, D3D12_RESOURCE_FLAG_NONE, D3D12_TEXTURE_LAYOUT_UNKNOWN, D3D12_HEAP_FLAG_NONE, &FD3DW::keep(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)), 1u);

	m_pShadowAtlas = FD3DW::FResource::CreateAnonimTexture(device, 1, NSOfflineConfig::Shadows::AtlasFormat, NSOfflineConfig::Shadows::AtlasWidth, NSOfflineConfig::Shadows::AtlasHeight, DXGI_SAMPLE_DESC({ 1,0 }), D3D12_RESOURCE_DIMENSION_TEXTURE2D, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_TEXTURE_LAYOUT_UNKNOWN, D3D12_HEAP_FLAG_NONE, &FD3DW::keep(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)), 1u);

	m_pAtlasPack = std::make_unique<FD3DW::SRV_UAVPacker>(GetCBV_SRV_UAVDescriptorSize(device), 6u, 0, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, device);

	m_pAtlasPack->AddResource(m_pTexelToLight->GetResource(), D3D12_SRV_DIMENSION_TEXTURE2D, 0, device);
	m_pAtlasPack->AddNullResource(1, device);
	m_pAtlasPack->AddNullResource(2, device);

	FD3DW::UAVResourceDesc desc;
	desc.Resource = m_pShadowAtlas->GetResource();
    desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    desc.Format = m_pShadowAtlas->GetResource()->GetDesc().Format;
    desc.MipSlice = 0;
    desc.PlaneSlice = 0;
    
    m_pAtlasPack->AddResource(desc, 3, device);
	
    FD3DW::UAVResourceDesc nullBlockerDistance{};
	nullBlockerDistance.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	nullBlockerDistance.Format = NSOfflineConfig::Shadows::AtlasFormat;
	
    m_pAtlasPack->AddResource(nullBlockerDistance, 4, device);
	
    FD3DW::UAVResourceDesc nullBlockerValidity{};
	nullBlockerValidity.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	nullBlockerValidity.Format = DXGI_FORMAT_R8_UINT;
	
    m_pAtlasPack->AddResource(nullBlockerValidity, 5, device);

    m_vTexelMap.resize(NSOfflineConfig::Shadows::AtlasWidth * NSOfflineConfig::Shadows::AtlasHeight, std::numeric_limits<NSOfflineConfig::Shadows::AtlasLightIndex>::max());
    auto recipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_DIRECT, [this](ID3D12GraphicsCommandList* list) {
		m_pTexelToLight->UploadData(m_pServices->GetDevice(), list, m_vTexelMap.data(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    });
    
    GlobalRenderThreadManager::GetInstance()->Submit(recipe);

}

void ShadowAtlasRenderer::ProcessNotify(NRenderSystemNotifyType type) {
    if (type == NRenderSystemNotifyType::Light || type == NRenderSystemNotifyType::LightUpdateData || type == NRenderSystemNotifyType::CameraInfoChanged || type==NRenderSystemNotifyType::CameraActivationDeactivation) {
		m_bIsNeedUpdateDataInBuffer.store(true, std::memory_order_release);
    }
    else if (type == NRenderSystemNotifyType::UpdateTLAS) {
        m_bIsNeedCheckEmptyAtlas.store(true, std::memory_order_release);
    }
}

void ShadowAtlasRenderer::SetHardShadowAtlasFilterSettings(const HardShadowAtlasFilterSettings& settings) {
	m_xHardBackend.SetFilterSettings(settings);
}

std::shared_ptr<FD3DW::ExecutionHandle> ShadowAtlasRenderer::OnCreateLightsMeta(std::vector<std::shared_ptr<FD3DW::ExecutionHandle>> sync) {

    return GlobalRenderThreadManager::GetInstance()->SubmitLambda([this]() {

        if (m_bIsNeedUpdateDataInBuffer.exchange(false, std::memory_order_acq_rel)) {
            const auto& data = m_pServices->GetLightComponentsData();

			std::vector<Rect> rects;
            auto count = int(data.size());
            rects.reserve(count);
            for (int i = 0; i < count; ++i) {
                int s = ComputeShadowSize(data[i]);
                rects.emplace_back(-1, -1, s, s, i);
            }

            auto updateRect = m_xAtlasPacker.SyncRects(rects);
            m_vMetas.resize(count);
            for (const auto& r : updateRect.Removed) {
                for (int yy = 0; yy < r.H; ++yy) {
                    int ay = r.Y + yy;
                    for (int xx = 0; xx < r.W; ++xx) {
                        int ax = r.X + xx;
                        uint32_t idx = ay * NSOfflineConfig::Shadows::AtlasWidth + ax;
                        m_vTexelMap[idx] = std::numeric_limits<NSOfflineConfig::Shadows::AtlasLightIndex>::max();
                    }
                }
                m_vDirtyRegions.push_back(r);
            }

            for (const auto& r : updateRect.Added) {
                if (r.W == 0) {
                    m_vMetas[r.ID].AtlasWidth = 0;
                    continue;
                }
                
                auto ox = r.X;
                auto oy = r.Y;

                m_vMetas[r.ID].LightIndex = r.ID;
                m_vMetas[r.ID].AtlasOffsetX = ox;
                m_vMetas[r.ID].AtlasOffsetY = oy;
                m_vMetas[r.ID].AtlasWidth = r.W;
                m_vMetas[r.ID].AtlasHeight = r.H;

                m_vMetas[r.ID].ScreenMinU = 0.0f;
                m_vMetas[r.ID].ScreenMinV = 0.0f;
                m_vMetas[r.ID].ScreenMaxU = 1.0f;
                m_vMetas[r.ID].ScreenMaxV = 1.0f;

                for (int yy = 0; yy < r.H; ++yy) {
                    int ay = r.Y + yy;
                    for (int xx = 0; xx < r.W; ++xx) {
                        int ax = r.X + xx;
                        auto idx = ay * NSOfflineConfig::Shadows::AtlasWidth + ax;
                        m_vTexelMap[idx] = static_cast<NSOfflineConfig::Shadows::AtlasLightIndex>(r.ID);
                    }
                }
                m_vDirtyRegions.push_back(r);
            }
			m_uLightsMetaCount.store(static_cast<UINT>(m_vMetas.size()), std::memory_order_release);
            m_bIsNeedUploadDataInBuffer.store(true, std::memory_order_release);
        }
    }, sync, true, true);
}

std::shared_ptr<FD3DW::ExecutionHandle> ShadowAtlasRenderer::OnUploadLightsMeta(std::shared_ptr<FD3DW::ExecutionHandle> sync) {
    std::shared_ptr<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>> uploadRecipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_DIRECT, [this](ID3D12GraphicsCommandList* list) {
        if (m_bIsNeedUploadDataInBuffer.exchange(false, std::memory_order_acq_rel)) {

			m_pLightAtlasMetaBuffer->UploadData(m_pServices->GetDevice(), list, m_vMetas.data(), int(m_vMetas.size()), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

            if (!m_vDirtyRegions.empty()) {
                auto device = m_pServices->GetDevice();
                for (const auto& r : m_vDirtyRegions) {
                    D3D12_BOX region = {};
                    region.left = r.X;
                    region.top = r.Y;
                    region.right = r.X + r.W;
                    region.bottom = r.Y + r.H;
                    region.front = 0;
                    region.back = 1;

                    m_pTexelToLight->UploadTextureRegion(
                        device,
                        list,
                        m_vTexelMap.data(),
                        0,
                        region,
                        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
                    );
                }
                m_vDirtyRegions.clear();
            }

        }
    });

    return GlobalRenderThreadManager::GetInstance()->Submit(uploadRecipe, { sync }, true);
}

std::shared_ptr<FD3DW::ExecutionHandle> ShadowAtlasRenderer::OnGenerateShadowAtlas(
	std::vector<std::shared_ptr<FD3DW::ExecutionHandle>> sync) {
	return m_xHardBackend.Schedule(*this, std::move(sync));
}

std::shared_ptr<FD3DW::ExecutionHandle> ShadowAtlasRenderer::ScheduleHardShadowAtlas(
	std::vector<std::shared_ptr<FD3DW::ExecutionHandle>> sync) {
    std::shared_ptr<FD3DW::CommandRecipe<ID3D12GraphicsCommandList4>> rtRecipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList4>>(D3D12_COMMAND_LIST_TYPE_DIRECT, [this](ID3D12GraphicsCommandList4* list) {
        auto* tlas = m_pServices->GetTLASResource();

		if (!tlas) {
			if (m_bIsNeedCheckEmptyAtlas.exchange(false, std::memory_order_acq_rel)) {
				m_pShadowAtlas->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				
                if (m_pBlockerDistanceAtlas) m_pBlockerDistanceAtlas->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				
                if (m_pBlockerValidityAtlas) m_pBlockerValidityAtlas->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

				ID3D12DescriptorHeap* heaps[] = { m_pAtlasPack->GetResult()->GetDescriptorPtr() };
				list->SetDescriptorHeaps(_countof(heaps), heaps);

				list->ClearUnorderedAccessViewFloat(m_pAtlasPack->GetResult()->GetGPUDescriptorHandle(3),m_pAtlasPack->GetResult()->GetCPUDescriptorHandle(3),m_pShadowAtlas->GetResource(),s_aEmptyVisibility,0,nullptr);
				if (m_pBlockerDistanceAtlas) {
					list->ClearUnorderedAccessViewFloat(m_pAtlasPack->GetResult()->GetGPUDescriptorHandle(4),m_pAtlasPack->GetResult()->GetCPUDescriptorHandle(4),m_pBlockerDistanceAtlas->GetResource(),s_aFullyLitVisibility,0,nullptr);
				}
				
                if (m_pBlockerValidityAtlas) {
					const UINT invalidFlags[4] = {};
					list->ClearUnorderedAccessViewUint(m_pAtlasPack->GetResult()->GetGPUDescriptorHandle(5),
						m_pAtlasPack->GetResult()->GetCPUDescriptorHandle(5), m_pBlockerValidityAtlas->GetResource(),
						invalidFlags, 0, nullptr);
				}
				
                m_pShadowAtlas->ResourceUAVBarrier(list);
				
                if (m_pBlockerDistanceAtlas) {
					m_pBlockerDistanceAtlas->ResourceUAVBarrier(list);
				}
				
                if (m_pBlockerValidityAtlas) {
					m_pBlockerValidityAtlas->ResourceUAVBarrier(list);
				}
			}

			m_pShadowAtlas->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			
            if (m_pBlockerDistanceAtlas) m_pBlockerDistanceAtlas->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			
            if (m_pBlockerValidityAtlas) m_pBlockerValidityAtlas->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			
            m_pLightAtlasMetaBuffer->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
            return;
        }

        AtlasRTShadowParams perFrameParams{};
        {
            std::lock_guard<std::mutex> lock(m_xHardBackend.m_xShadowParamsMutex);
		    
            m_xHardBackend.m_xShadowParams.ScreenWidth = std::max(1, m_pServices->GetSceneRenderWidth());
		    m_xHardBackend.m_xShadowParams.ScreenHeight = std::max(1, m_pServices->GetSceneRenderHeight());
		    m_xHardBackend.m_xShadowParams.InverseViewProjectionMatrix = dx::XMMatrixInverse(nullptr, dx::XMMatrixTranspose(m_pServices->GetViewProjectionMatrix()));
            
            perFrameParams = m_xHardBackend.m_xShadowParams;
        }
        m_pAtlasPerFrameDataBuffer->CpyData(0, perFrameParams);

        PSOManager::GetInstance()->GetPSOObject(PSOType::AtlasRTShadowDefaultConfig)->Bind(list);

        m_pAtlasPack->AddResource(m_pServices->GetCurrentDepthResource(), 1, m_pServices->GetDevice());

        list->SetComputeRootShaderResourceView(ATLAS_RT_SHADOW_ACC_DXR_BUFFER_POS_IN_ROOT_SIG, tlas->GetGPUVirtualAddress());

        auto* depthBuffer = m_pServices->GetCurrentDepthResource();

        SAFE_ASSERT(m_pNormalBuffer, "ShadowAtlasRenderer: normal G-buffer is not bound");
        SAFE_ASSERT(depthBuffer, "ShadowAtlasRenderer: depth buffer is not available");

        m_pNormalBuffer->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        depthBuffer->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

        m_pLightAtlasMetaBuffer->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        m_pTexelToLight->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

        m_pShadowAtlas->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		
        if (m_pBlockerDistanceAtlas) m_pBlockerDistanceAtlas->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		
        if (m_pBlockerValidityAtlas) m_pBlockerValidityAtlas->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        ID3D12DescriptorHeap* r[1] = { m_pAtlasPack->GetResult()->GetDescriptorPtr() };
        list->SetDescriptorHeaps(ARRAYSIZE(r), r);

		list->SetComputeRootShaderResourceView(ATLAS_RT_SHADOW_LIGHTS_SRV_POS_IN_ROOT_SIG, m_pServices->GetLightsBufferAddress());
		list->SetComputeRootShaderResourceView(ATLAS_RT_SHADOW_LIGHTS_METAS_SRV_POS_IN_ROOT_SIG, m_pLightAtlasMetaBuffer->GetResource()->GetGPUVirtualAddress());

		list->SetComputeRootDescriptorTable(ATLAS_RT_SHADOW_SRV_DESCRIPTION_TABLE_POS_IN_ROOT_SIG, m_pAtlasPack->GetResult()->GetGPUDescriptorHandle(0));
		list->SetComputeRootDescriptorTable(ATLAS_RT_SHADOW_OUTPUT_UAV_TABLE_POS_IN_ROOT_SIG, m_pAtlasPack->GetResult()->GetGPUDescriptorHandle(3));

        list->SetComputeRootConstantBufferView(ATLAS_RT_SHADOW_ATLAS_CBV_POS_IN_ROOT_SIG, m_pAtlasPerFrameDataBuffer->GetGPULocation(0));

        list->DispatchRays(m_pHardShadowBindingTable->GetDispatchRaysDesc(NSOfflineConfig::Shadows::AtlasWidth, NSOfflineConfig::Shadows::AtlasHeight, 1));

		m_pShadowAtlas->ResourceUAVBarrier(list);
		
        if (m_pBlockerDistanceAtlas) {
			m_pBlockerDistanceAtlas->ResourceUAVBarrier(list);
		}
		
        if (m_pBlockerValidityAtlas) {
			m_pBlockerValidityAtlas->ResourceUAVBarrier(list);
		}

        m_pShadowAtlas->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		
        if (m_pBlockerDistanceAtlas) m_pBlockerDistanceAtlas->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		
        if (m_pBlockerValidityAtlas) m_pBlockerValidityAtlas->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        
        m_pLightAtlasMetaBuffer->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        m_pNormalBuffer->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        depthBuffer->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    });

    return GlobalRenderThreadManager::GetInstance()->Submit(rtRecipe, sync, true);
}

void ShadowAtlasRenderer::SetGBuffersResources(FD3DW::FResource* normal, ID3D12Device* device) {
    m_pNormalBuffer = normal;

    m_pAtlasPack->AddResource(normal->GetResource(), D3D12_SRV_DIMENSION_TEXTURE2D, 2, device);
}

FD3DW::FResource* ShadowAtlasRenderer::GetShadowAtlas() const {
    return m_pShadowAtlas.get();
}

FD3DW::FResource* ShadowAtlasRenderer::GetBlockerDistanceAtlas() const {
	return m_pBlockerDistanceAtlas.get();
}

FD3DW::FResource* ShadowAtlasRenderer::GetBlockerValidityAtlas() const {
	return m_pBlockerValidityAtlas.get();
}

void ShadowAtlasRenderer::EnsureBlockerDistanceAtlas() {
	if (m_pBlockerDistanceAtlas && m_pBlockerValidityAtlas) return;
	
    auto* device = m_pServices->GetDevice();
	
    if (!m_pBlockerDistanceAtlas) m_pBlockerDistanceAtlas = FD3DW::FResource::CreateAnonimTexture(device, 1, NSOfflineConfig::Shadows::AtlasFormat, NSOfflineConfig::Shadows::AtlasWidth, NSOfflineConfig::Shadows::AtlasHeight, DXGI_SAMPLE_DESC({ 1,0 }), D3D12_RESOURCE_DIMENSION_TEXTURE2D, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_TEXTURE_LAYOUT_UNKNOWN, D3D12_HEAP_FLAG_NONE, &FD3DW::keep(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)), 1u);
	
    FD3DW::UAVResourceDesc desc{};
	desc.Resource = m_pBlockerDistanceAtlas->GetResource();
	desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	desc.Format = m_pBlockerDistanceAtlas->GetResource()->GetDesc().Format;
	
    m_pAtlasPack->AddResource(desc, 4, device);
	m_pBlockerValidityAtlas = FD3DW::FResource::CreateAnonimTexture(device, 1, DXGI_FORMAT_R8_UINT, NSOfflineConfig::Shadows::AtlasWidth, NSOfflineConfig::Shadows::AtlasHeight, DXGI_SAMPLE_DESC({ 1,0 }), D3D12_RESOURCE_DIMENSION_TEXTURE2D, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_TEXTURE_LAYOUT_UNKNOWN, D3D12_HEAP_FLAG_NONE, &FD3DW::keep(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)), 1u);
	
    FD3DW::UAVResourceDesc validityDesc{};
	validityDesc.Resource = m_pBlockerValidityAtlas->GetResource();
	validityDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	validityDesc.Format = DXGI_FORMAT_R8_UINT;
	
    m_pAtlasPack->AddResource(validityDesc, 5, device);
	
    m_bIsNeedCheckEmptyAtlas.store(true, std::memory_order_release);
}

D3D12_GPU_VIRTUAL_ADDRESS ShadowAtlasRenderer::GetLightsMetasBufferGPULocation() const {
    return m_pLightAtlasMetaBuffer->GetResource()->GetGPUVirtualAddress();
}

FD3DW::FResource* ShadowAtlasRenderer::GetLightsMetasBufferResource() const {
	return m_pLightAtlasMetaBuffer.get();
}

UINT ShadowAtlasRenderer::GetLightsMetaCount() const {
	return m_uLightsMetaCount.load(std::memory_order_acquire);
}

void ShadowAtlasRenderer::PrepareLightsMetaForCompute(ID3D12GraphicsCommandList* list) {
	m_pLightAtlasMetaBuffer->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
}

void ShadowAtlasRenderer::PrepareLightsMetaForPixel(ID3D12GraphicsCommandList* list) {
	m_pLightAtlasMetaBuffer->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
}

int ShadowAtlasRenderer::ComputeShadowSize(const LightComponentData& light) {
    auto camPos = m_pServices->GetCurrentCameraPosition();
    auto zFar = m_pServices->GetCameraFar();

    auto dist = FD3DW::hlsl::length(Subtract(light.Position, camPos));
    auto lod = FD3DW::hlsl::saturate(1.0f - dist / zFar);
    const int MinSize = static_cast<int>(NSOfflineConfig::Shadows::MinimumTileWidth);
    const int MaxSize = static_cast<int>(NSOfflineConfig::Shadows::MaximumTileWidth);

    auto sizeF = FD3DW::hlsl::lerp((float)MinSize,(float)MaxSize,lod);
    auto size = 1 << (int)std::round( std::log2(sizeF) );
    size = std::clamp(size, MinSize, MaxSize);

    return size;
}
