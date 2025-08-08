#pragma once

#include <Lights/MainRenderer_ShadowsComponent.h>
#include <D3DFramework/GraphicUtilites/RTShaderBindingTable.h>
#include <D3DFramework/GraphicUtilites/BilateralFilter.h>

class MainRenderer;


struct RTSoftShadowBuffer {
	dx::XMMATRIX PrevViewProj;
	dx::XMMATRIX CurrViewProj;
	float TemporalFeedbackMin = 0.02f;
	float TemporalFeedbackMax = 0.2f;
	float ReprojDistThreshold = 0.02f;
	float NormalThreshold = 0.9f;
};

class MainRenderer_RTSoftShadowsComponent : public MainRenderer_ShadowsComponent {
public:
	MainRenderer_RTSoftShadowsComponent() = default;
	virtual ~MainRenderer_RTSoftShadowsComponent() = default;


	virtual void AfterConstruction() override;
	virtual void BeforeRender(ID3D12GraphicsCommandList* list) override;
	virtual bool IsCanBeEnabled(MainRenderer* renderer) override;
	virtual void BeforeGBufferPass() override;
	virtual void AfterGBufferPass() override;
	virtual D3D12_SRV_DIMENSION GetSRVResultDimension() override;

	void SetGBuffersResources(FD3DW::FResource* worldPos, FD3DW::FResource* normal, ID3D12Device* device);
	
	virtual FD3DW::FResource* GetResultResource() override;
public:
	BEGIN_FIELD_REGISTRATION(MainRenderer_RTSoftShadowsComponent, MainRendererComponent)
	END_FIELD_REGISTRATION();


private:
	RTSoftShadowBuffer m_xBufferData;
	std::unique_ptr<FD3DW::UploadBuffer<RTSoftShadowBuffer>> m_pFrameBuffer;
	std::unique_ptr<FD3DW::RTShaderBindingTable> m_pSoftShadowsSBT;
	std::unique_ptr<FD3DW::SRV_UAVPacker> m_pSoftShadowsUAVPacker;
	std::unique_ptr<FD3DW::BilateralFilter> m_pBilateralFilter;

	int m_iCurrentShadowBufferUsage = 0;

	std::vector<std::unique_ptr<FD3DW::FResource>> m_vInOutWorldPosAndShadowInfo;

	std::unique_ptr<FD3DW::FResource> m_pSrcResultResource;
};