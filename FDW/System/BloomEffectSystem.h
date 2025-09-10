#pragma once

#include <pch.h>
#include <MainRenderer/MainRendererComponent.h>
#include <MainRenderer/GlobalRenderThreadManager.h>
#include <D3DFramework/GraphicUtilites/RenderTarget.h>
#include <D3DFramework/GraphicUtilites/ResourcePacker.h>
#include <D3DFramework/Objects/SimpleObjects.h>
#include <D3DFramework/Objects/ObjectVertexIndexDataCreator.h>
#include <System/BloomSystemConfigs.h>


class BloomEffectSystem : public MainRendererComponent {
public:
	BloomEffectSystem() = default;
	virtual ~BloomEffectSystem() = default;

public:
	virtual void AfterConstruction() override;

	std::shared_ptr<FD3DW::ExecutionHandle> ProcessBloomPass(std::shared_ptr<FD3DW::ExecutionHandle> handle);
	void SetShadingOutputResourceResultAndRect(FD3DW::FResource* shadingRes);

	bool IsEnabledBloom();
	void EnableBloom(bool b);
	ID3D12Resource* GetResultResource();

	BloomSystemCompositeData GetCompositeData();
	void SetCompositeData(BloomSystemCompositeData data);

	BloomSystemBrightPassData GetBrightPassData();
	void SetBrightPassData(BloomSystemBrightPassData data);

	BloomBlurType GetBloomBlurType();
	void SetBloomBlurType(BloomBlurType blurType);

protected:
	std::unique_ptr<FD3DW::RenderTarget> m_pBloomRTV;
	std::unique_ptr<FD3DW::RenderTarget> m_pBlurTransitRTV;
	std::unique_ptr<FD3DW::RenderTarget> m_pResultRTV;
	std::unique_ptr<FD3DW::RTVPacker> m_pBloomRTVPack;
	std::unique_ptr<FD3DW::SRV_UAVPacker> m_pBloomSRVPack;
	std::unique_ptr<FD3DW::UploadBuffer<BloomSystemBrightPassData>> m_pBrightPassDataBuffer;
	std::unique_ptr<FD3DW::UploadBuffer<BloomSystemBlurParams>> m_pBlurParamsBuffer;
	std::unique_ptr<FD3DW::UploadBuffer<BloomSystemCompositeData>> m_pCompositeDataBuffer;

	BloomSystemCompositeData m_xCompositeData;
	BloomSystemBrightPassData m_xBrightPassData;
	std::atomic<bool> m_bIsNeedUpdateCompositeData{ true };
	std::atomic<bool> m_bIsNeedUpdateBrightPassData{ true };

	std::unique_ptr<FD3DW::ObjectVertexIndexDataCreator<FD3DW::VertexFrameWork>> m_pSceneVBV_IBV;
	std::unique_ptr<FD3DW::Rectangle> m_pScreen = nullptr;
	FD3DW::FResource* InputScreen = nullptr;

	bool m_bIsBloomEnabled = false;
	BloomBlurType m_xBloomBlurType = BloomBlurType::Both;
};