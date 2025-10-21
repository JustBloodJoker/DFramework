#pragma once

#include <pch.h>
#include <MainRenderer/MainRendererComponent.h>
#include <D3DFramework/GraphicUtilites/StructuredBuffer.h>
#include <D3DFramework/GraphicUtilites/ResourcePacker.h>
#include <D3DFramework/GraphicUtilites/RTShaderBindingTable.h>
#include <System/AtlasRTShadowLightParams.h>
#include <Component/Light/LightComponentData.h>
#include <MainRenderer/GlobalRenderThreadManager.h>
#include <System/RectsPacker.h>

class AtlasRTShadowSystem : public MainRendererComponent {
public:
	AtlasRTShadowSystem() = default;
	virtual ~AtlasRTShadowSystem() = default;

public:
	virtual void AfterConstruction() override;
	virtual void ProcessNotify(NRenderSystemNotifyType type) override;

	std::shared_ptr<FD3DW::ExecutionHandle> OnCreateLightsMeta(std::vector<std::shared_ptr<FD3DW::ExecutionHandle>> sync);
	std::shared_ptr<FD3DW::ExecutionHandle> OnUploadLightsMeta(std::shared_ptr<FD3DW::ExecutionHandle> sync);
	std::shared_ptr<FD3DW::ExecutionHandle> OnGenerateShadowAtlas(std::vector<std::shared_ptr<FD3DW::ExecutionHandle>> sync);


	void SetGBuffersResources(FD3DW::FResource* worldPos, FD3DW::FResource* normal, ID3D12Device* device);

	FD3DW::FResource* GetShadowAtlas() const;
	D3D12_GPU_VIRTUAL_ADDRESS GetLightsMetasBufferGPULocation() const;
	D3D12_GPU_VIRTUAL_ADDRESS GetAtlasConstantBufferGPULocation() const;


protected:
	int ComputeShadowSize(const LightComponentData& L);
	LightAtlasRect ComputePointLightScreenRect(const dx::XMFLOAT3& lightPos, float attenuationRadius, int screenWidth, int screenHeight);

protected:
	std::unique_ptr<FD3DW::RTShaderBindingTable> m_pSoftShadowsSBT;

	std::vector<LightAtlasMeta> m_vMetas;
	std::vector<uint32_t> m_vTexelMap;
	std::vector<Rect> m_DirtyRegions;
	RectsPacker m_xAtlasPacker = RectsPacker(RT_SHADOW_ATLAS_MAX_WIDTH, RT_SHADOW_ATLAS_MAX_HEIGHT, 1);;

	std::unique_ptr<FD3DW::StructuredBuffer> m_pLightAtlasMetaBuffer = nullptr;

	std::unique_ptr<FD3DW::UploadBuffer<AtlasRTShadowParams>> m_pAtlasPerFrameDataBuffer = nullptr;

	std::unique_ptr<FD3DW::SRV_UAVPacker> m_pAtlasPack = nullptr;
	std::unique_ptr<FD3DW::FResource> m_pShadowAtlas = nullptr;
	std::unique_ptr<FD3DW::FResource> m_pTexelToLight = nullptr;
	

	std::atomic<bool> m_bIsNeedUpdateDataInBuffer{ true };
	std::atomic<bool> m_bIsNeedUploadDataInBuffer{ true };
};


inline dx::XMFLOAT3 operator-(const dx::XMFLOAT3& a, const dx::XMFLOAT3& b) {
	return dx::XMFLOAT3(a.x - b.x, a.y - b.y, a.z - b.z);
}