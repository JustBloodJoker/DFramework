#include "pch.h"

#include <System/Shadows/Atlas/ShadowAtlasRenderer.h>
#include <ShadowData/ShadowLightParameters.h>
#include <MainRenderer/PSOManager.h>
#include <Component/Light/LightComponentData.h>
#include <Config/RenderFormats.h>
#include <Config/ShaderBindings.h>
#include <Config/ShadowConfig.h>

ShadowAtlasRenderer::ShadowAtlasRenderer(IShadowRenderServices& services)
	: m_pServices(&services) {
}

dx::XMFLOAT3 ShadowAtlasRenderer::Subtract(const dx::XMFLOAT3& left, const dx::XMFLOAT3& right) {
    return dx::XMFLOAT3(left.x - right.x, left.y - right.y, left.z - right.z);
}

void ShadowAtlasRenderer::Initialize() {
	auto device = m_pServices->GetDevice();
	auto dxrDevice = m_pServices->GetDXRDevice();

    auto pso = PSOManager::GetInstance()->GetPSOObjectAs<FD3DW::RTPipelineObject>(PSOType::AtlasRTShadowDefaultConfig);
    m_pSoftShadowsSBT = std::make_unique<FD3DW::RTShaderBindingTable>(pso);
    m_pSoftShadowsSBT->InitSBT(dxrDevice);

	auto perLightRTPso = PSOManager::GetInstance()->GetPSOObjectAs<FD3DW::RTPipelineObject>(PSOType::PerLightRTShadowDefaultConfig);
	m_pPerLightRTSBT = std::make_unique<FD3DW::RTShaderBindingTable>(perLightRTPso);
	m_pPerLightRTSBT->InitSBT(dxrDevice);

	m_xMetadataProvider.m_pLightAtlasMetaBuffer = FD3DW::StructuredBuffer::CreateStructuredBuffer<LightAtlasMeta>(device, 1, true, D3D12_RESOURCE_FLAG_NONE, D3D12_HEAP_TYPE_DEFAULT);


    {
        std::lock_guard<std::mutex> lock(m_xHardBackend.m_xShadowParamsMutex);
        m_xHardBackend.m_xShadowParams.AtlasHeight = FDWConfig::Shadows::AtlasHeight;
        m_xHardBackend.m_xShadowParams.AtlasWidth = FDWConfig::Shadows::AtlasWidth;
        m_xHardBackend.m_xShadowParams.ScreenHeight = std::max(1, m_pServices->GetSceneRenderHeight());
        m_xHardBackend.m_xShadowParams.ScreenWidth = std::max(1, m_pServices->GetSceneRenderWidth());
    }

	m_pAtlasPerFrameDataBuffer = std::make_unique<FD3DW::UploadBuffer<AtlasRTShadowParams>>(device, 1, true);
	m_pPerLightRTPerFrameDataBuffer = std::make_unique<FD3DW::UploadBuffer<PerLightRTReferenceParams>>(device, 1, true);
    {
        std::lock_guard<std::mutex> lock(m_xHardBackend.m_xShadowParamsMutex);
	    m_pAtlasPerFrameDataBuffer->CpyData(0, m_xHardBackend.m_xShadowParams);

		PerLightRTReferenceParams referenceParams{};
		referenceParams.Atlas = m_xHardBackend.m_xShadowParams;
		
		HardShadowAtlasFilterSettings referenceSettings{};
		referenceSettings.Mode = HardShadowAtlasFilterMode::Nearest;
		referenceSettings.BlackLevel = 0.0f;
		referenceSettings.ShadowContrast = 1.0f;
		referenceSettings.NoiseScale = 0.0f;
		referenceParams.Atlas.SetFilterSettings(referenceSettings);
		referenceParams.Quality = ShadowLightParameters::BuildQualityMetadata(m_xPerLightRTBackend.m_xQualitySettings,0u, NSRTOutputDomain::AtlasBackedPreview);
		m_pPerLightRTPerFrameDataBuffer->CpyData(0, referenceParams);
    }

    m_pTexelToLight = FD3DW::FResource::CreateAnonimTexture(device, 1, FDWConfig::Shadows::AtlasLightIndexFormat, FDWConfig::Shadows::AtlasWidth, FDWConfig::Shadows::AtlasHeight, DXGI_SAMPLE_DESC({ 1,0 }), D3D12_RESOURCE_DIMENSION_TEXTURE2D, D3D12_RESOURCE_FLAG_NONE, D3D12_TEXTURE_LAYOUT_UNKNOWN, D3D12_HEAP_FLAG_NONE, &FD3DW::keep(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)), 1u);

	m_pShadowAtlas = FD3DW::FResource::CreateAnonimTexture(device, 1, FDWConfig::Shadows::AtlasFormat, FDWConfig::Shadows::AtlasWidth, FDWConfig::Shadows::AtlasHeight, DXGI_SAMPLE_DESC({ 1,0 }), D3D12_RESOURCE_DIMENSION_TEXTURE2D, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_TEXTURE_LAYOUT_UNKNOWN, D3D12_HEAP_FLAG_NONE, &FD3DW::keep(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)), 1u);

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
	nullBlockerDistance.Format = FDWConfig::Shadows::AtlasFormat;
	m_pAtlasPack->AddResource(nullBlockerDistance, 4, device);
	
	FD3DW::UAVResourceDesc nullBlockerValidity{};
	nullBlockerValidity.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	nullBlockerValidity.Format = DXGI_FORMAT_R8_UINT;
	m_pAtlasPack->AddResource(nullBlockerValidity, 5, device);

    m_xAtlasAllocator.m_vTexelMap.resize(FDWConfig::Shadows::AtlasWidth * FDWConfig::Shadows::AtlasHeight, std::numeric_limits<FDWConfig::Shadows::AtlasLightIndex>::max());
    
	auto recipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_DIRECT, [this](ID3D12GraphicsCommandList* list) {
		m_pTexelToLight->UploadData(m_pServices->GetDevice(), list, m_xAtlasAllocator.m_vTexelMap.data(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
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

HardShadowAtlasFilterSettings ShadowAtlasRenderer::GetHardShadowAtlasFilterSettings() const {
	return m_xHardBackend.GetFilterSettings();
}

void ShadowAtlasRenderer::SetHardShadowAtlasFilterSettings(const HardShadowAtlasFilterSettings& settings) {
	m_xHardBackend.SetFilterSettings(settings);
}

LightAtlasRect ShadowAtlasRenderer::ComputePointLightScreenRect(const dx::XMFLOAT3& lightPos, float attenuationRadius, int screenWidth, int screenHeight) {
    LightAtlasRect rect{};
    rect.Visible = true;
    rect.MinUV = { 0,0 };
    rect.MaxUV = { 1,1 };
    return rect;
}

std::shared_ptr<FD3DW::ExecutionHandle> ShadowAtlasRenderer::OnCreateLightsMeta(std::vector<std::shared_ptr<FD3DW::ExecutionHandle>> sync) {

    return GlobalRenderThreadManager::GetInstance()->SubmitLambda([this]() {

        if (m_bIsNeedUpdateDataInBuffer.exchange(false, std::memory_order_acq_rel)) {
            const auto& data = m_pServices->GetLightComponentsData();

			std::vector<Rect> rects;
            auto count = int(data.size());
            rects.reserve(count);
            
			auto cameraPos = m_pServices->GetCurrentCameraPosition();
            auto cameraZFar = m_pServices->GetCameraZFar();
            for (int i = 0; i < count; ++i) {
                int s = ComputeShadowSize(data[i]);
                rects.emplace_back(-1, -1, s, s, i);
            }

			std::vector<LightAtlasRect> lightSliceRects;
            lightSliceRects.resize(count);
			
			auto sceneWidth = std::max(1, m_pServices->GetSceneRenderWidth());
			auto sceneHeight = std::max(1, m_pServices->GetSceneRenderHeight());

            for(int i=0;i<count;++i) {
				lightSliceRects[i] = ComputePointLightScreenRect(data[i].Position, data[i].AttenuationRadius, sceneWidth, sceneHeight);
			}

            auto updateRect = m_xAtlasAllocator.m_xAtlasPacker.SyncRects(rects);

            m_xAtlasAllocator.m_vMetas.resize(count);
            for (const auto& r : updateRect.Removed) {
                for (int yy = 0; yy < r.H; ++yy) {
                    
					int ay = r.Y + yy;
                    for (int xx = 0; xx < r.W; ++xx) {
                        int ax = r.X + xx;
                        uint32_t idx = ay * FDWConfig::Shadows::AtlasWidth + ax;
                        m_xAtlasAllocator.m_vTexelMap[idx] = std::numeric_limits<FDWConfig::Shadows::AtlasLightIndex>::max();
                    }
                
				}
                m_xAtlasAllocator.m_vDirtyRegions.push_back(r);
            }

            for (const auto& r : updateRect.Added) {
                if (r.W == 0) {
                    m_xAtlasAllocator.m_vMetas[r.ID].AtlasWidth = 0;
                    continue;
                }
                auto ox = r.X;
                auto oy = r.Y;

                m_xAtlasAllocator.m_vMetas[r.ID].LightIndex = r.ID;
                m_xAtlasAllocator.m_vMetas[r.ID].AtlasOffsetX = ox;
                m_xAtlasAllocator.m_vMetas[r.ID].AtlasOffsetY = oy;
                m_xAtlasAllocator.m_vMetas[r.ID].AtlasWidth = r.W;
                m_xAtlasAllocator.m_vMetas[r.ID].AtlasHeight = r.H;

				auto& lightSliceRect = lightSliceRects[r.ID];
                m_xAtlasAllocator.m_vMetas[r.ID].ScreenMinU = lightSliceRect.MinUV.x;
                m_xAtlasAllocator.m_vMetas[r.ID].ScreenMinV = lightSliceRect.MinUV.y;
                m_xAtlasAllocator.m_vMetas[r.ID].ScreenMaxU = lightSliceRect.MaxUV.x;
                m_xAtlasAllocator.m_vMetas[r.ID].ScreenMaxV = lightSliceRect.MaxUV.y;

                auto isVisible = lightSliceRect.Visible;
                for (int yy = 0; yy < r.H; ++yy) {
                    
					int ay = r.Y + yy;
                    for (int xx = 0; xx < r.W; ++xx) {
                        int ax = r.X + xx;
                        auto idx = ay * FDWConfig::Shadows::AtlasWidth + ax;
                        m_xAtlasAllocator.m_vTexelMap[idx] = isVisible ? static_cast<FDWConfig::Shadows::AtlasLightIndex>(r.ID) : std::numeric_limits<FDWConfig::Shadows::AtlasLightIndex>::max();
                    }
                
				}
                m_xAtlasAllocator.m_vDirtyRegions.push_back(r);
            }
			m_uLightsMetaCount.store(static_cast<UINT>(m_xAtlasAllocator.m_vMetas.size()), std::memory_order_release);

            m_bIsNeedUploadDataInBuffer.store(true, std::memory_order_release);
        }
    }, sync, true, true);
}

std::shared_ptr<FD3DW::ExecutionHandle> ShadowAtlasRenderer::OnUploadLightsMeta(std::shared_ptr<FD3DW::ExecutionHandle> sync) {
    
	std::shared_ptr<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>> uploadRecipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_DIRECT, [this](ID3D12GraphicsCommandList* list) {
        if (m_bIsNeedUploadDataInBuffer.exchange(false, std::memory_order_acq_rel)) {

			m_xMetadataProvider.m_pLightAtlasMetaBuffer->UploadData(m_pServices->GetDevice(), list, m_xAtlasAllocator.m_vMetas.data(), int(m_xAtlasAllocator.m_vMetas.size()), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

            if (!m_xAtlasAllocator.m_vDirtyRegions.empty()) {
                
				auto device = m_pServices->GetDevice();
                for (const auto& r : m_xAtlasAllocator.m_vDirtyRegions) {
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
                        m_xAtlasAllocator.m_vTexelMap.data(),
                        0,
                        region,
                        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
                    );
                }
                m_xAtlasAllocator.m_vDirtyRegions.clear();
            }

        }
    });

    return GlobalRenderThreadManager::GetInstance()->Submit(uploadRecipe, { sync }, true);
}

std::shared_ptr<FD3DW::ExecutionHandle> ShadowAtlasRenderer::OnGenerateShadowAtlas(std::vector<std::shared_ptr<FD3DW::ExecutionHandle>> sync) {
	return m_xHardBackend.Schedule(*this, std::move(sync));
}

std::shared_ptr<FD3DW::ExecutionHandle> ShadowAtlasRenderer::ScheduleHardShadowAtlas(std::vector<std::shared_ptr<FD3DW::ExecutionHandle>> sync) {
    
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
				
				if (m_pBlockerDistanceAtlas) m_pBlockerDistanceAtlas->ResourceUAVBarrier(list);
				
				if (m_pBlockerValidityAtlas) m_pBlockerValidityAtlas->ResourceUAVBarrier(list);
			}

			m_pShadowAtlas->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			
			if (m_pBlockerDistanceAtlas) m_pBlockerDistanceAtlas->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			
			if (m_pBlockerValidityAtlas) m_pBlockerValidityAtlas->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			
			m_xMetadataProvider.m_pLightAtlasMetaBuffer->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
            return;
        }

        AtlasRTShadowParams perFrameParams{};
        {
            std::lock_guard<std::mutex> lock(m_xHardBackend.m_xShadowParamsMutex);
		    m_xHardBackend.m_xShadowParams.ScreenWidth = std::max(1, m_pServices->GetSceneRenderWidth());
		    m_xHardBackend.m_xShadowParams.ScreenHeight = std::max(1, m_pServices->GetSceneRenderHeight());
		    m_xHardBackend.m_xShadowParams.InverseViewProjectionMatrix = dx::XMMatrixInverse(nullptr, dx::XMMatrixTranspose(m_pServices->GetJitteredViewProjectionMatrix()));
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

        m_xMetadataProvider.m_pLightAtlasMetaBuffer->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        m_pTexelToLight->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

        m_pShadowAtlas->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		
		if (m_pBlockerDistanceAtlas) m_pBlockerDistanceAtlas->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		
		if (m_pBlockerValidityAtlas) m_pBlockerValidityAtlas->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        ID3D12DescriptorHeap* r[1] = { m_pAtlasPack->GetResult()->GetDescriptorPtr() };
        list->SetDescriptorHeaps(ARRAYSIZE(r), r);

		list->SetComputeRootShaderResourceView(ATLAS_RT_SHADOW_LIGHTS_SRV_POS_IN_ROOT_SIG, m_pServices->GetLightsBufferAddress());
		list->SetComputeRootShaderResourceView(ATLAS_RT_SHADOW_LIGHTS_METAS_SRV_POS_IN_ROOT_SIG, m_xMetadataProvider.m_pLightAtlasMetaBuffer->GetResource()->GetGPUVirtualAddress());

		list->SetComputeRootDescriptorTable(ATLAS_RT_SHADOW_SRV_DESCRIPTION_TABLE_POS_IN_ROOT_SIG, m_pAtlasPack->GetResult()->GetGPUDescriptorHandle(0));
		list->SetComputeRootDescriptorTable(ATLAS_RT_SHADOW_OUTPUT_UAV_TABLE_POS_IN_ROOT_SIG, m_pAtlasPack->GetResult()->GetGPUDescriptorHandle(3));

        list->SetComputeRootConstantBufferView(ATLAS_RT_SHADOW_ATLAS_CBV_POS_IN_ROOT_SIG, m_pAtlasPerFrameDataBuffer->GetGPULocation(0));

        list->DispatchRays(m_pSoftShadowsSBT->GetDispatchRaysDesc(FDWConfig::Shadows::AtlasWidth, FDWConfig::Shadows::AtlasHeight, 1));

		m_pShadowAtlas->ResourceUAVBarrier(list);
		
		if (m_pBlockerDistanceAtlas) m_pBlockerDistanceAtlas->ResourceUAVBarrier(list);
		
		if (m_pBlockerValidityAtlas) m_pBlockerValidityAtlas->ResourceUAVBarrier(list);

        m_pShadowAtlas->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		
		if (m_pBlockerDistanceAtlas) m_pBlockerDistanceAtlas->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		
		if (m_pBlockerValidityAtlas) m_pBlockerValidityAtlas->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        
		m_xMetadataProvider.m_pLightAtlasMetaBuffer->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        m_pNormalBuffer->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        depthBuffer->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    });

	auto handle = GlobalRenderThreadManager::GetInstance()->Submit(rtRecipe, sync, true);
    return handle;
}

void ShadowAtlasRenderer::SetGBuffersResources(FD3DW::FResource* normal, ID3D12Device* device) {
    m_pNormalBuffer = normal;

    m_pAtlasPack->AddResource(normal->GetResource(), D3D12_SRV_DIMENSION_TEXTURE2D, 2, device);
}

std::shared_ptr<FD3DW::ExecutionHandle> ShadowAtlasRenderer::OnGeneratePerLightRTVisibility(std::vector<std::shared_ptr<FD3DW::ExecutionHandle>> sync) {
	return m_xPerLightRTBackend.Schedule(*this, std::move(sync));
}

std::shared_ptr<FD3DW::ExecutionHandle> ShadowAtlasRenderer::SchedulePerLightRTVisibility(std::vector<std::shared_ptr<FD3DW::ExecutionHandle>> sync) {
	
	NSRTQualitySettings qualitySettings{};
	{
		std::lock_guard<std::mutex> lock(m_xPerLightRTBackend.m_xQualitySettingsMutex);
		qualitySettings = m_xPerLightRTBackend.m_xQualitySettings;
	}
	
	qualitySettings = ShadowLightParameters::SanitizeQualitySettings(qualitySettings);
	
	const auto scheduledGeneration = qualitySettings.Mode == NSRTQualityMode::RuntimeReference
		? m_xPerLightRTBackend.m_uNextPerLightRTGeneration.fetch_add(1, std::memory_order_relaxed)
		: qualitySettings.OfflineGenerationId;
	
	const auto qualityMetadata = ShadowLightParameters::BuildQualityMetadata(qualitySettings, scheduledGeneration, NSRTOutputDomain::AtlasBackedPreview);
	
	const auto& lights = m_pServices->GetLightComponentsData();
	const auto shadowSettings = m_pServices->GetShadowLightSettings();
	
	std::vector<ShadowLightCandidate> selectionCandidates;
	selectionCandidates.reserve(lights.size());
	
	const auto cameraPosition = m_pServices->GetCurrentCameraPosition();
	for (std::size_t lightIndex = 0; lightIndex < lights.size(); ++lightIndex) {
	
		const auto& light = lights[lightIndex];
		
		const auto type = static_cast<NLightComponentTypes>(light.LightType);
		const float dx = light.Position.x - cameraPosition.x;
		const float dy = light.Position.y - cameraPosition.y;
		const float dz = light.Position.z - cameraPosition.z;

		const float distance = type == NLightComponentTypes::DirectionalLight ? 0.0f : std::sqrt(dx * dx + dy * dy + dz * dz);
		const float influence = type == NLightComponentTypes::DirectionalLight? 1.0f : std::max(0.0f, light.AttenuationRadius) /std::max(1.0f, distance + std::max(0.0f, light.AttenuationRadius));
		
		selectionCandidates.push_back({
			static_cast<std::uint64_t>(lightIndex),
			static_cast<std::uint32_t>(lightIndex),
			static_cast<std::uint32_t>(light.LightType),
			lightIndex < shadowSettings.size() ? shadowSettings[lightIndex].CastsShadows : true,
			lightIndex < shadowSettings.size() ? shadowSettings[lightIndex].Priority : 0.0f,
			influence,
			light.Intensity,
			light.AttenuationRadius,
			distance });
	}
	const auto selection = m_xPerLightRTBackend.m_xLightSelector.Select(selectionCandidates, scheduledGeneration);

	auto rtRecipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList4>>( D3D12_COMMAND_LIST_TYPE_DIRECT,
		[this, qualityMetadata, selectedLights = selection.SelectedGlobalLightIndices](ID3D12GraphicsCommandList4* list) {
			
			auto frameQualityMetadata = qualityMetadata;
			
			const auto geometry = m_pServices->GetRTGeometryCoverage();
			frameQualityMetadata.GeometryCoverageFlags = geometry.Flags;
			frameQualityMetadata.UnsupportedGeometryInstanceCount = geometry.ConservativeAlphaOrOpacityInstances + geometry.ProceduralInstances;
			frameQualityMetadata.MissingAccelerationStructureInstanceCount = geometry.MissingBLASInstances + (geometry.TLASComplete != 0u ? 0u : 1u);
			frameQualityMetadata.IsCanonicalGroundTruthPreset = 0u;
			
			auto* tlas = m_pServices->GetTLASResource();
			if (!tlas) {
				m_pShadowAtlas->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

				ID3D12DescriptorHeap* heaps[] = { m_pAtlasPack->GetResult()->GetDescriptorPtr() };
				list->SetDescriptorHeaps(_countof(heaps), heaps);
				list->ClearUnorderedAccessViewFloat(
					m_pAtlasPack->GetResult()->GetGPUDescriptorHandle(3),
					m_pAtlasPack->GetResult()->GetCPUDescriptorHandle(3),
					m_pShadowAtlas->GetResource(),
					s_aFullyLitVisibility,
					0,
					nullptr);

				m_pShadowAtlas->ResourceUAVBarrier(list);
				m_pShadowAtlas->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
				m_xMetadataProvider.m_pLightAtlasMetaBuffer->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
				return;
			}

			PerLightRTReferenceParams referenceParams{};
			for (auto& meta : m_xAtlasAllocator.m_vMetas) meta.padd1 = 0u;
			
			for (const auto lightIndex : selectedLights) {
				if (lightIndex < m_xAtlasAllocator.m_vMetas.size()) m_xAtlasAllocator.m_vMetas[lightIndex].padd1 = 1u;
			}
			
			m_xMetadataProvider.m_pLightAtlasMetaBuffer->UploadData(
				m_pServices->GetDevice(),
				list,
				m_xAtlasAllocator.m_vMetas.data(),
				static_cast<int>(m_xAtlasAllocator.m_vMetas.size()),
				D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
			
			{
				std::lock_guard<std::mutex> lock(m_xHardBackend.m_xShadowParamsMutex);
				referenceParams.Atlas = m_xHardBackend.m_xShadowParams;
			}
			
			referenceParams.Atlas.ScreenWidth = std::max(1, m_pServices->GetSceneRenderWidth());
			referenceParams.Atlas.ScreenHeight = std::max(1, m_pServices->GetSceneRenderHeight());
			referenceParams.Atlas.InverseViewProjectionMatrix = dx::XMMatrixInverse( nullptr, dx::XMMatrixTranspose(m_pServices->GetJitteredViewProjectionMatrix()));
			
			HardShadowAtlasFilterSettings referenceSettings{};
			referenceSettings.Mode = HardShadowAtlasFilterMode::Nearest;
			referenceSettings.BlackLevel = 0.0f;
			referenceSettings.ShadowContrast = 1.0f;
			referenceSettings.NoiseScale = 0.0f;
			
			referenceParams.Atlas.SetFilterSettings(referenceSettings);
			referenceParams.Quality = frameQualityMetadata;
			
			m_pPerLightRTPerFrameDataBuffer->CpyData(0, referenceParams);

			PSOManager::GetInstance()->GetPSOObject(PSOType::PerLightRTShadowDefaultConfig)->Bind(list);
			
			m_pAtlasPack->AddResource(m_pServices->GetCurrentDepthResource(), 1, m_pServices->GetDevice());
			list->SetComputeRootShaderResourceView(ATLAS_RT_SHADOW_ACC_DXR_BUFFER_POS_IN_ROOT_SIG, tlas->GetGPUVirtualAddress());

			auto* depthBuffer = m_pServices->GetCurrentDepthResource();
			SAFE_ASSERT(m_pNormalBuffer, "ShadowAtlasRenderer: normal G-buffer is not bound");
			SAFE_ASSERT(depthBuffer, "ShadowAtlasRenderer: depth buffer is not available");

			m_pNormalBuffer->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			depthBuffer->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			m_xMetadataProvider.m_pLightAtlasMetaBuffer->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
			m_pTexelToLight->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			m_pShadowAtlas->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

			ID3D12DescriptorHeap* heaps[] = { m_pAtlasPack->GetResult()->GetDescriptorPtr() };
			list->SetDescriptorHeaps(_countof(heaps), heaps);
			list->SetComputeRootShaderResourceView(ATLAS_RT_SHADOW_LIGHTS_SRV_POS_IN_ROOT_SIG, m_pServices->GetLightsBufferAddress());
			list->SetComputeRootShaderResourceView(ATLAS_RT_SHADOW_LIGHTS_METAS_SRV_POS_IN_ROOT_SIG, m_xMetadataProvider.m_pLightAtlasMetaBuffer->GetResource()->GetGPUVirtualAddress());
			list->SetComputeRootDescriptorTable(ATLAS_RT_SHADOW_SRV_DESCRIPTION_TABLE_POS_IN_ROOT_SIG, m_pAtlasPack->GetResult()->GetGPUDescriptorHandle(0));
			list->SetComputeRootDescriptorTable(ATLAS_RT_SHADOW_OUTPUT_UAV_TABLE_POS_IN_ROOT_SIG, m_pAtlasPack->GetResult()->GetGPUDescriptorHandle(3));
			list->SetComputeRootConstantBufferView(ATLAS_RT_SHADOW_ATLAS_CBV_POS_IN_ROOT_SIG, m_pPerLightRTPerFrameDataBuffer->GetGPULocation(0));

			list->DispatchRays(m_pPerLightRTSBT->GetDispatchRaysDesc(FDWConfig::Shadows::AtlasWidth, FDWConfig::Shadows::AtlasHeight, 1));

			m_pShadowAtlas->ResourceUAVBarrier(list);
			m_pShadowAtlas->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			m_xMetadataProvider.m_pLightAtlasMetaBuffer->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
			m_pNormalBuffer->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			depthBuffer->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		});

	return GlobalRenderThreadManager::GetInstance()->Submit(rtRecipe, sync, true);
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
	if (!m_pBlockerDistanceAtlas) m_pBlockerDistanceAtlas = FD3DW::FResource::CreateAnonimTexture(device, 1, FDWConfig::Shadows::AtlasFormat, FDWConfig::Shadows::AtlasWidth, FDWConfig::Shadows::AtlasHeight, DXGI_SAMPLE_DESC({ 1,0 }), D3D12_RESOURCE_DIMENSION_TEXTURE2D, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_TEXTURE_LAYOUT_UNKNOWN, D3D12_HEAP_FLAG_NONE, &FD3DW::keep(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)), 1u);
	
	FD3DW::UAVResourceDesc desc{};
	desc.Resource = m_pBlockerDistanceAtlas->GetResource();
	desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	desc.Format = m_pBlockerDistanceAtlas->GetResource()->GetDesc().Format;
	
	m_pAtlasPack->AddResource(desc, 4, device);
	
	m_pBlockerValidityAtlas = FD3DW::FResource::CreateAnonimTexture(device, 1, DXGI_FORMAT_R8_UINT, FDWConfig::Shadows::AtlasWidth, FDWConfig::Shadows::AtlasHeight, DXGI_SAMPLE_DESC({ 1,0 }), D3D12_RESOURCE_DIMENSION_TEXTURE2D, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_TEXTURE_LAYOUT_UNKNOWN, D3D12_HEAP_FLAG_NONE, &FD3DW::keep(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)), 1u);
	
	FD3DW::UAVResourceDesc validityDesc{};
	validityDesc.Resource = m_pBlockerValidityAtlas->GetResource();
	validityDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	validityDesc.Format = DXGI_FORMAT_R8_UINT;
	
	m_pAtlasPack->AddResource(validityDesc, 5, device);
	
	m_bIsNeedCheckEmptyAtlas.store(true, std::memory_order_release);
}

D3D12_GPU_VIRTUAL_ADDRESS ShadowAtlasRenderer::GetLightsMetasBufferGPULocation() const {
    return m_xMetadataProvider.m_pLightAtlasMetaBuffer->GetResource()->GetGPUVirtualAddress();
}

FD3DW::FResource* ShadowAtlasRenderer::GetLightsMetasBufferResource() const {
	return m_xMetadataProvider.m_pLightAtlasMetaBuffer.get();
}

UINT ShadowAtlasRenderer::GetLightsMetaCount() const {
	return m_uLightsMetaCount.load(std::memory_order_acquire);
}

void ShadowAtlasRenderer::PrepareLightsMetaForCompute(ID3D12GraphicsCommandList* list) {
	m_xMetadataProvider.m_pLightAtlasMetaBuffer->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
}

void ShadowAtlasRenderer::PrepareLightsMetaForPixel(ID3D12GraphicsCommandList* list) {
	m_xMetadataProvider.m_pLightAtlasMetaBuffer->ResourceBarrierChange(list, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
}

D3D12_GPU_VIRTUAL_ADDRESS ShadowAtlasRenderer::GetAtlasConstantBufferGPULocation() const {
    return m_pAtlasPerFrameDataBuffer->GetGPULocation(0);
}

D3D12_GPU_VIRTUAL_ADDRESS ShadowAtlasRenderer::GetPerLightRTConstantBufferGPULocation() const {
	return m_pPerLightRTPerFrameDataBuffer->GetGPULocation(0);
}

std::uint64_t ShadowAtlasRenderer::GetCpuTexelMirrorBytes() const {
	return static_cast<std::uint64_t>(m_xAtlasAllocator.m_vTexelMap.capacity()) * sizeof(FDWConfig::Shadows::AtlasLightIndex);
}

bool ShadowAtlasRenderer::IsPerLightRTReady() const {
	return m_pPerLightRTSBT && m_pPerLightRTPerFrameDataBuffer && m_xMetadataProvider.m_pLightAtlasMetaBuffer && m_pAtlasPack && m_pShadowAtlas && m_pTexelToLight;
}

NSRTQualitySettings ShadowAtlasRenderer::GetNSRTQualitySettings() const {
	return m_xPerLightRTBackend.GetQualitySettings();
}

void ShadowAtlasRenderer::SetNSRTQualitySettings(const NSRTQualitySettings& settings) {
	m_xPerLightRTBackend.SetQualitySettings(settings);
}

int ShadowAtlasRenderer::ComputeShadowSize(const LightComponentData& light) {
    auto camPos = m_pServices->GetCurrentCameraPosition();
    auto zFar = m_pServices->GetCameraZFar();

    auto dist = FD3DW::hlsl::length(Subtract(light.Position, camPos));
    auto lod = FD3DW::hlsl::saturate(1.0f - dist / zFar);
   
	const int MinSize = static_cast<int>(FDWConfig::Shadows::MinimumTileWidth);
    const int MaxSize = static_cast<int>(FDWConfig::Shadows::MaximumTileWidth);

    auto sizeF = FD3DW::hlsl::lerp((float)MinSize,(float)MaxSize,lod);
    auto size = 1 << (int)std::round( std::log2(sizeF) );
    size = std::clamp(size, MinSize, MaxSize);

    return size;
}
