#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <vector>
#include <System/NRenderSystemNotifyType.h>
#include <System/Shadows/ShadowRenderServices.h>
#include <D3DFramework/GraphicUtilites/RenderThreadUtils/ExecutionHandle.h>
#include <D3DFramework/GraphicUtilites/StructuredBuffer.h>
#include <D3DFramework/GraphicUtilites/ResourcePacker.h>
#include <D3DFramework/GraphicUtilites/RTShaderBindingTable.h>
#include <ShadowData/ShadowLightParameters.h>
#include <System/Shadows/RayTracing/RTGeometryCoverage.h>
#include <Component/Light/LightComponentData.h>
#include <Config/ShadowConfig.h>
#include <Renderer/GlobalRenderThreadManager.h>
#include <System/RectsPacker.h>
#include <System/Shadows/Atlas/HardShadowAtlasBackend.h>

class ShadowAtlasRenderer {
	
	friend class HardShadowAtlasBackend;

public:
	explicit ShadowAtlasRenderer(IShadowRenderServices& services);
	~ShadowAtlasRenderer() = default;

public:
	void Initialize();
	void ProcessNotify(NRenderSystemNotifyType type);

	std::shared_ptr<FD3DW::ExecutionHandle> OnCreateLightsMeta(std::vector<std::shared_ptr<FD3DW::ExecutionHandle>> sync);
	std::shared_ptr<FD3DW::ExecutionHandle> OnUploadLightsMeta(std::shared_ptr<FD3DW::ExecutionHandle> sync);
	std::shared_ptr<FD3DW::ExecutionHandle> OnGenerateShadowAtlas(std::vector<std::shared_ptr<FD3DW::ExecutionHandle>> sync);

    void SetHardShadowAtlasFilterSettings(const HardShadowAtlasFilterSettings& settings);

	void SetGBuffersResources(FD3DW::FResource* normal, ID3D12Device* device);

	FD3DW::FResource* GetShadowAtlas() const;
	FD3DW::FResource* GetBlockerDistanceAtlas() const;
	FD3DW::FResource* GetBlockerValidityAtlas() const;
	
	void EnsureBlockerDistanceAtlas();
	
	D3D12_GPU_VIRTUAL_ADDRESS GetLightsMetasBufferGPULocation() const;
	FD3DW::FResource* GetLightsMetasBufferResource() const;
	UINT GetLightsMetaCount() const;
	
	void PrepareLightsMetaForCompute(ID3D12GraphicsCommandList* list);
	void PrepareLightsMetaForPixel(ID3D12GraphicsCommandList* list);


protected:
	inline static constexpr float s_aEmptyVisibility[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	inline static constexpr float s_aFullyLitVisibility[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

protected:
	static dx::XMFLOAT3 Subtract(const dx::XMFLOAT3& left, const dx::XMFLOAT3& right);
	int ComputeShadowSize(const LightComponentData& light);

protected:
	std::vector<LightAtlasMeta> m_vMetas;
	std::vector<NSOfflineConfig::Shadows::AtlasLightIndex> m_vTexelMap;
	std::vector<Rect> m_vDirtyRegions;
	
	RectsPacker m_xAtlasPacker{
		static_cast<int>(NSOfflineConfig::Shadows::AtlasWidth),
		static_cast<int>(NSOfflineConfig::Shadows::AtlasHeight),
		1 };

	std::unique_ptr<FD3DW::StructuredBuffer> m_pLightAtlasMetaBuffer;
	
	HardShadowAtlasBackend m_xHardBackend;
	
	IShadowRenderServices* m_pServices = nullptr;
	
	std::shared_ptr<FD3DW::ExecutionHandle> ScheduleHardShadowAtlas(std::vector<std::shared_ptr<FD3DW::ExecutionHandle>> sync);

protected:
	std::unique_ptr<FD3DW::RTShaderBindingTable> m_pHardShadowBindingTable;

	std::unique_ptr<FD3DW::UploadBuffer<AtlasRTShadowParams>> m_pAtlasPerFrameDataBuffer = nullptr;

	std::unique_ptr<FD3DW::SRV_UAVPacker> m_pAtlasPack = nullptr;
	std::unique_ptr<FD3DW::FResource> m_pShadowAtlas = nullptr;
	std::unique_ptr<FD3DW::FResource> m_pBlockerDistanceAtlas = nullptr;
	std::unique_ptr<FD3DW::FResource> m_pBlockerValidityAtlas = nullptr;
	std::unique_ptr<FD3DW::FResource> m_pTexelToLight = nullptr;
	
	FD3DW::FResource* m_pNormalBuffer = nullptr;
	
	std::atomic<bool> m_bIsNeedUpdateDataInBuffer{ true };
	std::atomic<bool> m_bIsNeedCheckEmptyAtlas{ true };
	std::atomic<bool> m_bIsNeedUploadDataInBuffer{ true };
	std::atomic<UINT> m_uLightsMetaCount{ 0u };
};
