#pragma once

#include <pch.h>
#include <MainRenderer/MainRendererComponent.h>
#include <D3DFramework/GraphicUtilites/RenderTarget.h>
#include <Entity/RenderObject/SkyboxComponent.h>



struct SkyboxRenderPassInput {
	D3D12_CPU_DESCRIPTOR_HANDLE DSV_CPU;
	D3D12_CPU_DESCRIPTOR_HANDLE RTV_CPU;
	D3D12_RECT Rect;
	D3D12_VIEWPORT Viewport;
};


class SkyboxRenderSystem : public MainRendererComponent {
public:
	SkyboxRenderSystem() = default;
	virtual ~SkyboxRenderSystem() = default;

public:

	virtual void AfterConstruction() override;
	virtual std::shared_ptr<FD3DW::ExecutionHandle> OnStartRenderTick(std::shared_ptr<FD3DW::ExecutionHandle> sync);
	virtual std::shared_ptr<FD3DW::ExecutionHandle> RenderSkyboxPass(std::shared_ptr<FD3DW::ExecutionHandle> sync, SkyboxRenderPassInput input);
	virtual void ProcessNotify(NRenderSystemNotifyType type) override;


protected:
	std::atomic<bool> m_bIsNeedUpdateActiveSkybox{ true };
	SkyboxComponent* m_pActiveComponent = nullptr;
	std::unique_ptr<FD3DW::UploadBuffer<FD3DW::MatricesConstantBufferStructureFrameWork>> m_pMatricesBuffer;
};