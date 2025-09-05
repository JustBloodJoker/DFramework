#pragma once

#include <pch.h>
#include <MainRenderer/MainRendererComponent.h>
#include <D3DFramework/GraphicUtilites/BufferManager.h>
#include <D3DFramework/GraphicUtilites/StructuredBuffer.h>
#include <D3DFramework/GraphicUtilites/RenderThreadUtils/ExecutionHandle.h>
#include <Component/Light/LightComponent.h>


struct LightSystemBuffer {
	int LightCount;
	dx::XMFLOAT3 CameraPos;
	int IsShadowImpl;
	dx::XMFLOAT3 margin;
};

class LightSystem : public MainRendererComponent {
public:
	LightSystem() = default;
	virtual ~LightSystem() = default;

public:
	virtual void AfterConstruction() override;
	std::shared_ptr<FD3DW::ExecutionHandle> OnStartRenderTick(std::shared_ptr<FD3DW::ExecutionHandle> syncHandle);

public:
	virtual void ProcessNotify(NRenderSystemNotifyType type) override;

	int GetLightsCount();

protected:
	std::vector<LightComponentData> GetDataFromLightComponents(std::vector<LightComponent*> cmps);

protected:
	LightSystemBuffer m_xLightBuffer;

	std::atomic<bool> m_bIsNeedUpdateDataInBuffer{ true };

	std::unique_ptr<FD3DW::UploadBuffer<LightSystemBuffer>> m_pLightsHelperConstantBuffer;
	std::unique_ptr<FD3DW::StructuredBuffer> m_pLightsStructuredBuffer;

};