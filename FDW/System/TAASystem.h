#pragma once

#include <pch.h>
#include <MainRenderer/MainRendererComponent.h>
#include <MainRenderer/GlobalRenderThreadManager.h>
#include <D3DFramework/GraphicUtilites/RenderTarget.h>
#include <D3DFramework/GraphicUtilites/ResourcePacker.h>
#include <D3DFramework/Objects/SimpleObjects.h>
#include <D3DFramework/Objects/ObjectVertexIndexDataCreator.h>
#include <System/TAASystemData.h>


class TAASystem : public MainRendererComponent {
public:
	TAASystem() = default;
	virtual ~TAASystem() = default;

public:
	virtual void AfterConstruction() override;
	void SetGBufferResources(FD3DW::FResource* sceneShading, FD3DW::FResource* motion, FD3DW::DepthStencilView* dsv1, FD3DW::DepthStencilView* dsv2);
	void ResizeResources(UINT width, UINT height);

	std::shared_ptr<FD3DW::ExecutionHandle> ProcessTAABufferCollection(std::vector<std::shared_ptr<FD3DW::ExecutionHandle>> sync);

	FD3DW::FResource* GetCurrentResultResource();
	bool IsTAAEnabled();
	void EnableTAA(bool b);

protected:
	std::unique_ptr<FD3DW::RenderTarget> m_pHistoryBuffers[2];

	std::unique_ptr<FD3DW::RTVPacker> m_pRTVPack;
	std::unique_ptr<FD3DW::SRV_UAVPacker> m_pSRVPack;

	std::unique_ptr<FD3DW::UploadBuffer<TAASystemData>> m_pDataBuffer;

	std::unique_ptr<FD3DW::ObjectVertexIndexDataCreator<FD3DW::VertexFrameWork>> m_pSceneVBV_IBV;
	std::unique_ptr<FD3DW::Rectangle> m_pScreen = nullptr;

	bool m_bIsTAAEnabled = true;
};
