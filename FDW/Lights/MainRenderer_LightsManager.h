#pragma once

#include <pch.h>
#include <D3DFramework/GraphicUtilites/BufferManager.h>
#include <MainRenderer/MainRendererComponent.h>
#include <D3DFramework/GraphicUtilites/StructuredBuffer.h>
#include <Lights/LightStruct.h>

class MainRenderer;

class MainRenderer_LightsManager : public MainRendererComponent {

public:
	MainRenderer_LightsManager(MainRenderer* owner);
	virtual ~MainRenderer_LightsManager() = default;

	virtual void AfterConstruction() override;

	void AddLight(LightStruct light);
	void DeleteLight(int idx);

	const LightStruct& GetLight(int idx);
	void SetLightData(LightStruct newData, int idx);
	int GetLightsCount();

public:
	void BeforeRender(ID3D12GraphicsCommandList* list);

public:
	void BindLightConstantBuffer(UINT cbSlot, UINT rootSRVSlot, ID3D12GraphicsCommandList* list);

private:
	void UpdateLightsConstantBuffer();
	void UpdateLightsStructuredBuffer(ID3D12GraphicsCommandList* list);
	
	LightBuffer m_xLightBuffer;
	std::vector<LightStruct> m_vLights;

	bool m_bIsNeedUpdateLightsStructuredBuffer = false;

	std::unique_ptr<FD3DW::UploadBuffer<LightBuffer>> m_pLightsHelperConstantBuffer;
	std::unique_ptr<FD3DW::StructuredBuffer> m_pLightsStructuredBuffer;

};