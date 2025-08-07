#pragma once

#include <Lights/MainRenderer_ShadowsComponent.h>
#include <D3DFramework/GraphicUtilites/RTShaderBindingTable.h>

class MainRenderer;

class MainRenderer_RTSoftShadowsComponent : public MainRenderer_ShadowsComponent {
public:
	MainRenderer_RTSoftShadowsComponent() = default;
	virtual ~MainRenderer_RTSoftShadowsComponent() = default;


	virtual void AfterConstruction() override;
	virtual bool IsCanBeEnabled(MainRenderer* renderer) override;
	virtual void BeforeGBufferPass() override;
	virtual void AfterGBufferPass() override;
	virtual D3D12_SRV_DIMENSION GetSRVResultDimension() override;

	void SetGBuffersResources(FD3DW::FResource* worldPos, FD3DW::FResource* normal, ID3D12Device* device);

public:
	BEGIN_FIELD_REGISTRATION(MainRenderer_RTSoftShadowsComponent, MainRendererComponent)
	END_FIELD_REGISTRATION();


private:

	std::unique_ptr<FD3DW::RTShaderBindingTable> m_pSoftShadowsSBT;
	std::unique_ptr<FD3DW::SRV_UAVPacker> m_pSoftShadowsUAVPacker;

};