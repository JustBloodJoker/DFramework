#include <System/AtlasRTShadowSystem.h>
#include <MainRenderer/MainRenderer.h>
#include <World/World.h>
#include <System/AtlasRTShadowLightParams.h>
#include <MainRenderer/PSOManager.h>
#include <Component/Light/LightComponentData.h>

void AtlasRTShadowSystem::AfterConstruction() {
	auto device = m_pOwner->GetDevice();
	auto dxrDevice = m_pOwner->GetDXRDevice();

    auto pso = PSOManager::GetInstance()->GetPSOObjectAs<FD3DW::RTPipelineObject>(PSOType::AtlasRTShadowDefaultConfig);
    m_pSoftShadowsSBT = std::make_unique<FD3DW::RTShaderBindingTable>(pso);
    m_pSoftShadowsSBT->InitSBT(dxrDevice);

	m_pLightAtlasMetaBuffer = FD3DW::StructuredBuffer::CreateStructuredBuffer<LightAtlasMeta>(device, 1, true, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_HEAP_TYPE_DEFAULT);

    AtlasRTShadowParams params;
	params.AtlasHeight = RT_SHADOW_ATLAS_MAX_HEIGHT;
	params.AtlasWidth = RT_SHADOW_ATLAS_MAX_WIDTH;
	params.ScreenHeight = m_pOwner->WNDSettings().Height;
	params.ScreenWidth = m_pOwner->WNDSettings().Width;

	m_pAtlasPerFrameDataBuffer = std::make_unique<FD3DW::UploadBuffer<AtlasRTShadowParams>>(device, 1, true);
	m_pAtlasPerFrameDataBuffer->CpyData(0, params);

    m_pTexelToLight = FD3DW::FResource::CreateAnonimTexture(device, 1, RT_SHADOW_ATLAS_LIGHT_IDX_FORMAT, RT_SHADOW_ATLAS_MAX_WIDTH, RT_SHADOW_ATLAS_MAX_HEIGHT, DXGI_SAMPLE_DESC({ 1,0 }), D3D12_RESOURCE_DIMENSION_TEXTURE2D, D3D12_RESOURCE_FLAG_NONE, D3D12_TEXTURE_LAYOUT_UNKNOWN, D3D12_HEAP_FLAG_NONE, &FD3DW::keep(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)), 1u);

	m_pShadowAtlas = FD3DW::FResource::CreateAnonimTexture(device, 1, RT_SHADOW_ATLAS_FORMAT, RT_SHADOW_ATLAS_MAX_WIDTH, RT_SHADOW_ATLAS_MAX_HEIGHT, DXGI_SAMPLE_DESC({ 1,0 }), D3D12_RESOURCE_DIMENSION_TEXTURE2D, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_TEXTURE_LAYOUT_UNKNOWN, D3D12_HEAP_FLAG_NONE, &FD3DW::keep(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)), 1u);

	m_pAtlasPack = std::make_unique<FD3DW::SRV_UAVPacker>(GetCBV_SRV_UAVDescriptorSize(device), 4u, 0, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, device);

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

    m_vTexelMap.resize(RT_SHADOW_ATLAS_MAX_WIDTH * RT_SHADOW_ATLAS_MAX_HEIGHT, 0xFFFFFFFFu);
    auto recipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_DIRECT, [this](ID3D12GraphicsCommandList* list) {
		m_pTexelToLight->UploadData(m_pOwner->GetDevice(), list, m_vTexelMap.data(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    });
    GlobalRenderThreadManager::GetInstance()->Submit(recipe);

}

void AtlasRTShadowSystem::ProcessNotify(NRenderSystemNotifyType type) {
    if (type == NRenderSystemNotifyType::Light || type == NRenderSystemNotifyType::LightUpdateData || type == NRenderSystemNotifyType::CameraInfoChanged || type==NRenderSystemNotifyType::CameraActivationDeactivation) {
		m_bIsNeedUpdateDataInBuffer.store(true, std::memory_order_release);
    }
    else if (type == NRenderSystemNotifyType::UpdateTLAS) {
        m_bIsNeedCheckEmptyAtlas.store(true, std::memory_order_release);
    }
}

LightAtlasRect AtlasRTShadowSystem::ComputePointLightScreenRect(const dx::XMFLOAT3& lightPos, float attenuationRadius, int screenWidth, int screenHeight) {
    LightAtlasRect rect{};
    rect.Visible = true;
    rect.MinUV = { 0,0 };
    rect.MaxUV = { 1,1 };
    return rect;
}

std::shared_ptr<FD3DW::ExecutionHandle> AtlasRTShadowSystem::OnCreateLightsMeta(std::vector<std::shared_ptr<FD3DW::ExecutionHandle>> sync) {
	
    return GlobalRenderThreadManager::GetInstance()->SubmitLambda([this]() {
        
        if (m_bIsNeedUpdateDataInBuffer.exchange(false, std::memory_order_acq_rel)) {
            const auto& data = m_pOwner->GetLightComponentsData();

			std::vector<Rect> rects;
            auto count = int(data.size());
            rects.reserve(count);
            auto cameraPos = m_pOwner->GetCurrentCameraPosition();
            auto cameraZFar = m_pOwner->GetCameraFrustum().GetZFar();
            for (int i = 0; i < count; ++i) {
                int s = ComputeShadowSize(data[i]);
                rects.emplace_back(-1, -1, s, s, i);
            }

			std::vector<LightAtlasRect> lightSliceRects;
            lightSliceRects.resize(count);
			auto wndSettings = m_pOwner->WNDSettings();
            
            for(int i=0;i<count;++i) {
				lightSliceRects[i] = ComputePointLightScreenRect(data[i].Position, data[i].AttenuationRadius, wndSettings.Width, wndSettings.Height);
			}

            auto updateRect = m_xAtlasPacker.SyncRects(rects);


            m_vMetas.resize(count);
            for (const auto& r : updateRect.Removed) {
                for (int yy = 0; yy < r.H; ++yy) {
                    int ay = r.Y + yy;
                    for (int xx = 0; xx < r.W; ++xx) {
                        int ax = r.X + xx;
                        uint32_t idx = ay * RT_SHADOW_ATLAS_MAX_WIDTH + ax;
                        m_vTexelMap[idx] = 0xFFFFFFFFu;
                    }
                }
                m_DirtyRegions.push_back(r);
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

				auto& lightSliceRect = lightSliceRects[r.ID];
                m_vMetas[r.ID].ScreenMinU = lightSliceRect.MinUV.x;
                m_vMetas[r.ID].ScreenMinV = lightSliceRect.MinUV.y;
                m_vMetas[r.ID].ScreenMaxU = lightSliceRect.MaxUV.x;
                m_vMetas[r.ID].ScreenMaxV = lightSliceRect.MaxUV.y;

                auto isVisible = lightSliceRect.Visible;
                for (int yy = 0; yy < r.H; ++yy) {
                    int ay = r.Y + yy;
                    for (int xx = 0; xx < r.W; ++xx) {
                        int ax = r.X + xx;
                        uint32_t idx = ay * RT_SHADOW_ATLAS_MAX_WIDTH + ax;
                        m_vTexelMap[idx] = isVisible ? (uint32_t)r.ID : 0xFFFFFFFFu;
                    }
                }
                m_DirtyRegions.push_back(r);
            }


            m_bIsNeedUploadDataInBuffer.store(true, std::memory_order_release);
        }
    }, sync, true, true);
}

std::shared_ptr<FD3DW::ExecutionHandle> AtlasRTShadowSystem::OnUploadLightsMeta(std::shared_ptr<FD3DW::ExecutionHandle> sync) {
    std::shared_ptr<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>> uploadRecipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_DIRECT, [this](ID3D12GraphicsCommandList* list) {
        if (m_bIsNeedUploadDataInBuffer.exchange(false, std::memory_order_acq_rel)) {

            m_pLightAtlasMetaBuffer->UploadDataNoBarrier(m_pOwner->GetDevice(), list, m_vMetas.data(), int(m_vMetas.size()));

            if (!m_DirtyRegions.empty()) {
                auto device = m_pOwner->GetDevice();
                for (const auto& r : m_DirtyRegions) {
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
                m_DirtyRegions.clear();
            }

        }
    });

    return GlobalRenderThreadManager::GetInstance()->Submit(uploadRecipe, { sync }, true);
}

std::shared_ptr<FD3DW::ExecutionHandle> AtlasRTShadowSystem::OnGenerateShadowAtlas(std::vector<std::shared_ptr<FD3DW::ExecutionHandle>> sync) {
    std::shared_ptr<FD3DW::CommandRecipe<ID3D12GraphicsCommandList4>> rtRecipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList4>>(D3D12_COMMAND_LIST_TYPE_DIRECT, [this](ID3D12GraphicsCommandList4* list) {
        auto tlas = m_pOwner->GetTLAS().pResult;

        if (!tlas) {
			if(m_bIsNeedCheckEmptyAtlas.exchange(false, std::memory_order_acq_rel) ) m_pShadowAtlas->ClearTexture(m_pOwner->GetDevice(), list, nullptr);
            return;
        }

        auto wndSettings = m_pOwner->GetMainWNDSettings();

        PSOManager::GetInstance()->GetPSOObject(PSOType::AtlasRTShadowDefaultConfig)->Bind(list);

        list->SetComputeRootShaderResourceView(ATLAS_RT_SHADOW_ACC_DXR_BUFFER_POS_IN_ROOT_SIG, tlas->GetGPUVirtualAddress());

        m_pLightAtlasMetaBuffer->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        m_pTexelToLight->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        m_pShadowAtlas->ResourceBarrierChange(list, 1, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        
        ID3D12DescriptorHeap* r[1] = { m_pAtlasPack->GetResult()->GetDescriptorPtr() };
        list->SetDescriptorHeaps(ARRAYSIZE(r), r);

		list->SetComputeRootShaderResourceView(ATLAS_RT_SHADOW_LIGHTS_SRV_POS_IN_ROOT_SIG, m_pOwner->GetLightsStructuredBufferAddress());
		list->SetComputeRootShaderResourceView(ATLAS_RT_SHADOW_LIGHTS_METAS_SRV_POS_IN_ROOT_SIG, m_pLightAtlasMetaBuffer->GetResource()->GetGPUVirtualAddress());
		
        list->SetComputeRootDescriptorTable(ATLAS_RT_SHADOW_SRV_DESCRIPTION_TABLE_POS_IN_ROOT_SIG, m_pAtlasPack->GetResult()->GetGPUDescriptorHandle(0));
		list->SetComputeRootDescriptorTable(ATLAS_RT_SHADOW_OUTPUT_UAV_TABLE_POS_IN_ROOT_SIG, m_pAtlasPack->GetResult()->GetGPUDescriptorHandle(3));
		
        list->SetComputeRootConstantBufferView(ATLAS_RT_SHADOW_ATLAS_CBV_POS_IN_ROOT_SIG, m_pAtlasPerFrameDataBuffer->GetGPULocation(0));

        list->DispatchRays(m_pSoftShadowsSBT->GetDispatchRaysDesc(RT_SHADOW_ATLAS_MAX_WIDTH, RT_SHADOW_ATLAS_MAX_HEIGHT, 1));
     });

    return GlobalRenderThreadManager::GetInstance()->Submit(rtRecipe, sync, false);
}

void AtlasRTShadowSystem::SetGBuffersResources(FD3DW::FResource* worldPos, FD3DW::FResource* normal, ID3D12Device* device) {
    m_pAtlasPack->AddResource(worldPos->GetResource(), D3D12_SRV_DIMENSION_TEXTURE2D, 1, device);
    m_pAtlasPack->AddResource(normal->GetResource(), D3D12_SRV_DIMENSION_TEXTURE2D, 2, device);
}

FD3DW::FResource* AtlasRTShadowSystem::GetShadowAtlas() const {
    return m_pShadowAtlas.get();
}

D3D12_GPU_VIRTUAL_ADDRESS AtlasRTShadowSystem::GetLightsMetasBufferGPULocation() const {
    return m_pLightAtlasMetaBuffer->GetResource()->GetGPUVirtualAddress();
}

D3D12_GPU_VIRTUAL_ADDRESS AtlasRTShadowSystem::GetAtlasConstantBufferGPULocation() const {
    return m_pAtlasPerFrameDataBuffer->GetGPULocation(0);
}

int AtlasRTShadowSystem::ComputeShadowSize(const LightComponentData& L) {
    auto camPos = m_pOwner->GetCurrentCameraPosition();
    auto  zFar = m_pOwner->GetCameraFrustum().GetZFar();

    auto dist = FD3DW::hlsl::length(L.Position - camPos);
    auto lod = FD3DW::hlsl::saturate(1.0f - dist / zFar);
    auto MinSize = RT_SHADOW_ATLAS_MIN_TILE_RESOLUTION_WIDTH;
    auto MaxSize = RT_SHADOW_ATLAS_MAX_TILE_RESOLUTION_WIDTH;

    auto sizeF = FD3DW::hlsl::lerp((float)MinSize,(float)MaxSize,lod);
    auto size = 1 << (int)std::round( std::log2(sizeF) );
    size = std::clamp(size, MinSize, MaxSize);

    return size;
}