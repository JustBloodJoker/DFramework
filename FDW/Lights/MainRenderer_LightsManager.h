#pragma once

#include <pch.h>
#include <D3DFramework/GraphicUtilites/BufferManager.h>
#include <MainRenderer/MainRendererComponent.h>
#include <D3DFramework/GraphicUtilites/StructuredBuffer.h>
#include <D3DFramework/GraphicUtilites/ResourcePacker.h>
#include <D3DFramework/GraphicUtilites/RenderThreadUtils/ExecutionHandle.h>
#include <Lights/LightStruct.h>
#include <Lights/ClusteredShadingPassData.h>

class MainRenderer;

class MainRenderer_LightsManager : public MainRendererComponent {

public:
	virtual ~MainRenderer_LightsManager() = default;

	virtual void AfterConstruction() override;

	virtual void InitLTC(ID3D12GraphicsCommandList* list, FD3DW::SRV_UAVPacker* srvPack);

	void AddLight(LightStruct light);
	void DeleteLight(int idx);

	const LightStruct& GetLight(int idx);
	void SetLightData(LightStruct newData, int idx);
	int GetLightsCount();

public:
	void BeforeRender(ID3D12GraphicsCommandList* list);
	std::shared_ptr<FD3DW::ExecutionHandle> ClusteredShadingPass(std::shared_ptr<FD3DW::ExecutionHandle> beforeRender);

public:
	void BindLightConstantBuffer(UINT cbLightsSlot, UINT rootSRVLightsSlot, UINT rootSRVClustersSlot, UINT cbClusterDataSlot, ID3D12GraphicsCommandList* list, bool IsCompute);

public:
	BEGIN_FIELD_REGISTRATION(MainRenderer_LightsManager, MainRendererComponent)
		REGISTER_FIELD(m_vLights)
	END_FIELD_REGISTRATION()

private:
	void UpdateLightsConstantBuffer();
	void UpdateLightsStructuredBuffer(ID3D12GraphicsCommandList* list);
	void UpdateLightsClustersData();


	LightBuffer m_xLightBuffer;
	std::vector<LightStruct> m_vLights;

	bool m_bIsNeedUpdateLightsStructuredBuffer = false;

	std::unique_ptr<FD3DW::UploadBuffer<LightBuffer>> m_pLightsHelperConstantBuffer;
	std::unique_ptr<FD3DW::StructuredBuffer> m_pLightsStructuredBuffer;

	std::vector< std::shared_ptr<FD3DW::FResource>> m_vLCTResources;

	std::unique_ptr<FD3DW::UploadBuffer<ClusterViewParams>> m_pClusterViewParamsBuffer;
	std::unique_ptr<FD3DW::UploadBuffer<ClusterParams>> m_pClusterParamsBuffer;
	std::unique_ptr<FD3DW::UploadBuffer<ClusterParamsPS>> m_pClusterParamsPSBuffer;
	std::unique_ptr<FD3DW::StructuredBuffer> m_pClustersStructuredBuffer;
};