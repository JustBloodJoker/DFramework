#pragma once

#include <pch.h>
#include <MainRenderer/MainRendererComponent.h>
#include <D3DFramework/GraphicUtilites/BufferManager.h>
#include <D3DFramework/GraphicUtilites/FResource.h>
#include <D3DFramework/GraphicUtilites/StructuredBuffer.h>
#include <D3DFramework/GraphicUtilites/RenderThreadUtils/ExecutionHandle.h>
#include <Component/Light/LightComponent.h>


struct LightSystemBuffer {
	int LightCount;
	dx::XMFLOAT3 CameraPos;
	int IsShadowImpl;    
	float ZNear;
	float ZFar;
	int FrameIndex;
	int IsIBLEnabled;
	float IBLDiffuseIntensity;
	float IBLSpecularIntensity;
	float IBLMaxReflectionMip;
	dx::XMMATRIX InverseViewProjectionMatrix;
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
	FD3DW::StructuredBuffer* GetLightsBuffer();

	D3D12_GPU_VIRTUAL_ADDRESS GetLightsStructuredBufferGPULocation();
	D3D12_GPU_VIRTUAL_ADDRESS GetLightsConstantBufferGPULocation();
	
	const std::vector<LightComponentData>& GetLightComponentsData() const;

public: //IBL
	FD3DW::FResource* GetIBLBrdfLUTResource();

	bool IsEnabledIBL();
	void EnableIBL(bool b);

	float GetIBLDiffuseIntensity() const;
	void SetIBLDiffuseIntensity(float value);

	float GetIBLSpecularIntensity() const;
	void SetIBLSpecularIntensity(float value);

	float GetIBLMaxReflectionMip() const;
	void SetIBLMaxReflectionMip(float value);

protected:
	float RadicalInverseVdC(UINT bits);
	dx::XMFLOAT2 Hammersley(UINT i, UINT n);
	dx::XMFLOAT3 ImportanceSampleGGX(const dx::XMFLOAT2& xi, float roughness);
	float GeometrySchlickGGX_IBL(float nDotV, float roughness);
	float GeometrySmith_IBL(float nDotV, float nDotL, float roughness);
    dx::XMFLOAT2 IntegrateBRDF(float nDotV, float roughness);
    std::shared_ptr<FD3DW::FResource> CreateIBLBrdfLutResource(ID3D12Device* device, ID3D12GraphicsCommandList* list);

protected:
	std::vector<LightComponentData> GetDataFromLightComponents(std::vector<LightComponent*> cmps);

protected:
	LightSystemBuffer m_xLightBuffer;
	std::vector<LightComponentData> m_vLightComponentsData;

	std::atomic<bool> m_bIsNeedUpdateDataInBuffer{ true };

	std::unique_ptr<FD3DW::UploadBuffer<LightSystemBuffer>> m_pLightsHelperConstantBuffer;
	std::unique_ptr<FD3DW::StructuredBuffer> m_pLightsStructuredBuffer;
	std::shared_ptr<FD3DW::FResource> m_pIBLBrdfLUTResource;

protected:
	bool m_bIsIBLEnabled = true;
	float m_fIBLDiffuseIntensity = 1.0f;
	float m_fIBLSpecularIntensity = 1.0f;
	float m_fIBLMaxReflectionMip = 8.0f;
};
