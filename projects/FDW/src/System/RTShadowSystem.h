#pragma once

#include <pch.h>
#include <MainRenderer/MainRendererComponent.h>
#include <MainRenderer/GlobalRenderThreadManager.h>
#include <System/RTShadowSystemConfig.h>
#include <D3DFramework/GraphicUtilites/RTShaderBindingTable.h>

class RTShadowSystem : public MainRendererComponent {
public:
	RTShadowSystem() = default;
	virtual ~RTShadowSystem() = default;


public:
	virtual void AfterConstruction() override;
	virtual void ProcessNotify(NRenderSystemNotifyType type) override;

	virtual std::shared_ptr<FD3DW::ExecutionHandle> OnStartRenderTick(std::shared_ptr<FD3DW::ExecutionHandle> handle);
	virtual std::shared_ptr<FD3DW::ExecutionHandle> OnRenderShadowFactors(std::vector<std::shared_ptr<FD3DW::ExecutionHandle>> handle);

	void SetGBuffersResources(FD3DW::FResource* worldPos, FD3DW::FResource* normal, ID3D12Device* device);

	virtual FD3DW::FResource* GetResultResource();

	RTShadowSystemConfig GetConfig();
	void SetConfig(RTShadowSystemConfig config);

protected:
	RTShadowSystemConfig m_xConfig;
	std::unique_ptr<FD3DW::UploadBuffer<RTShadowSystemBuffer>> m_pFrameBuffer;
	std::unique_ptr<FD3DW::RTShaderBindingTable> m_pSoftShadowsSBT;
	std::unique_ptr<FD3DW::SRV_UAVPacker> m_pSoftShadowsUAVPacker;
	std::unique_ptr<FD3DW::BilateralFilter> m_pBilateralFilter;

	int m_iCurrentShadowBufferUsage = 0;

	std::vector<std::unique_ptr<FD3DW::FResource>> m_vInOutWorldPosAndShadowInfo;

	std::unique_ptr<FD3DW::FResource> m_pSrcResultResource;
};