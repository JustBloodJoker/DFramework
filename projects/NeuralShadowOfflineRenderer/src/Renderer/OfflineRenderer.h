#pragma once

#include <Renderer/OfflineRendererComponent.h>
#include <Config/LightingConfig.h>
#include <Config/PipelineRegistry.h>
#include <Config/RendererLimits.h>
#include <Config/RenderFormats.h>
#include <Config/ShaderBindings.h>
#include <Config/ShadowConfig.h>
#include <Config/RendererResources.h>

#include <D3DFramework/D3DFW.h>
#include <Renderer/GlobalRenderThreadManager.h>

#include <D3DFramework/GraphicUtilites/RTShaderBindingTable.h>

#include <World/World.h>
#include <System/CameraSystem.h>
#include <System/LightSystem.h>
#include <System/ClusteredLightningSystem.h>
#include <System/Shadows/Neural/NeuralShadowWorkGenerator.h>
#include <System/Shadows/Neural/NeuralShadowBaselinePass.h>
#include <System/Shadows/ShadowSystem.h>
#include <System/RenderMeshesSystem.h>

class MeshComponent;
class NeuralShadowCaptureController;

class OfflineRenderer : virtual public FD3DW::D3DFW {
	
	friend class ShadowSystem;
	friend class NeuralShadowCaptureController;

public:
	
	explicit OfflineRenderer(NeuralShadowCaptureController& captureController);
	virtual ~OfflineRenderer()=default;

	void UserInit() override;
	void UserLoop() override;
	void UserClose()  override;
	FD3DW::BaseCommandQueue* UserSwapchainCommandQueue(ID3D12Device* device) override;

public:
	World* GetWorld();

public: //SIZE
	int GetSceneRenderWidth() const;
	int GetSceneRenderHeight() const;


public:
	FD3DW::DepthStencilView* GetCurrentDSV();
	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentDSV_CPUAddr();

public:
	///////////////////////////////////////////////////
	//BRIDGE METHODS

	//CAMERA
	dx::XMMATRIX GetCurrentProjectionMatrix() const;
	dx::XMMATRIX GetCurrentViewMatrix() const;
	dx::XMMATRIX GetViewProjectionMatrix() const;
	dx::XMFLOAT3 GetCurrentCameraPosition() const;
	CameraFrustum GetCameraFrustum();
	float GetCameraNear() const;
	float GetCameraFar() const;

	//MESHES
	FD3DW::AccelerationStructureBuffers GetTLAS();
	NSRTGeometryCoverageState GetRTGeometryCoverageState() const;
	NSRTGeometryAccounting GetRTGeometryAccounting() const;
	FD3DW::FResource* GetRTGeometryInstancesResource() const;
	ID3D12Resource* GetRTAlphaIndexBuffer() const;
	ID3D12Resource* GetRTAlphaVertexBuffer() const;


	//LIGHTS
	D3D12_GPU_VIRTUAL_ADDRESS GetClusterStructuredBufferAddress();
	FD3DW::StructuredBuffer* GetLightsBuffer();
	const std::vector<LightComponentData>& GetLightComponentsData() const;
	int GetLightsCount();

	//SHADOWS
	void SetHardShadowAtlasFilterSettings(const HardShadowAtlasFilterSettings& settings);
	UINT GetShadowMetadataCount() const;

	//WORLD
	void LoadWorld(std::string pathTo);

protected:
	std::shared_ptr<World> CreateEmptyWorld();

private:
	NeuralShadowCaptureController& m_xCaptureController;
	void ProcessNotifiesInWorld();
	void ProcessNotifies(std::vector<NRenderSystemNotifyType> notifies);
	void UpdateSceneViewportLayout();

private:
	void InitOfflineRendererParts(ID3D12Device* device);
	void InitOfflineRendererDXRParts(ID3D12Device5* device);

	void InitOfflineRendererSystems();
	void InitOfflineRendererDXRSystems(ID3D12Device5* device);

private:

	std::shared_ptr<World> m_pWorld;

	///////////////////////////////////////////
	CameraSystem* m_pCameraSystem = nullptr;
	LightSystem* m_pLightSystem = nullptr;
	ClusteredLightningSystem* m_pClusteredLightningSystem = nullptr;
	ShadowSystem* m_pShadowSystem = nullptr;
	RenderMeshesSystem* m_pRenderMeshesSystem = nullptr;
	std::vector<std::unique_ptr<OfflineRendererComponent>> m_vSystems;

protected:
	template <typename T, typename... Args>
	T* CreateSystem(Args&&... args) {
		auto cmp = std::make_unique<T>(std::forward<Args>(args)...);
		cmp->SetAfterConstruction(this);
		auto ptr = cmp.get();
		m_vSystems.push_back(std::move(cmp));
		return ptr;
	}

protected:
	std::vector<std::unique_ptr<FD3DW::RenderTarget>> m_pGBuffers;
	std::unique_ptr<FD3DW::RTVPacker> m_pGBuffersRTVPack;

	UINT m_uCurrentDSVIndex = 0;
	std::unique_ptr<FD3DW::DepthStencilView> m_pDSV[2];
	std::unique_ptr<FD3DW::DSVPacker> m_pDSVPack;

	D3D12_VIEWPORT m_xSceneViewPort = {};
	D3D12_RECT m_xSceneRect = {};

};
