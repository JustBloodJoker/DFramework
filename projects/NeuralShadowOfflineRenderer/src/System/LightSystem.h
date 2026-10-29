#pragma once

#include <Renderer/OfflineRendererComponent.h>
#include <D3DFramework/GraphicUtilites/BufferManager.h>
#include <D3DFramework/GraphicUtilites/StructuredBuffer.h>
#include <D3DFramework/GraphicUtilites/RenderThreadUtils/ExecutionHandle.h>
#include <Component/Light/LightComponent.h>

class LightSystem : public OfflineRendererComponent {
public:
	LightSystem() = default;
	virtual ~LightSystem() = default;

public:
	virtual void AfterConstruction() override;
	std::shared_ptr<FD3DW::ExecutionHandle> OnStartRenderTick(std::shared_ptr<FD3DW::ExecutionHandle> syncHandle);

public:
	virtual void ProcessNotify(NRenderSystemNotifyType type) override;

	int GetLightsCount();
	FD3DW::StructuredBuffer* GetLightsBuffer();

	const std::vector<LightComponentData>& GetLightComponentsData() const;

protected:
	std::vector<LightComponentData> GetDataFromLightComponents(std::vector<LightComponent*> cmps);

protected:
	std::vector<LightComponentData> m_vLightComponentsData;

	std::atomic<bool> m_bIsNeedUpdateDataInBuffer{ true };

	std::unique_ptr<FD3DW::StructuredBuffer> m_pLightsStructuredBuffer;
};
