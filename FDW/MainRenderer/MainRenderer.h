#pragma once

#include <pch.h>
#include <MainRenderer/MainRendererComponent.h>

#include <UI/MainRenderer_UIComponent.h>
#include <D3DFramework/D3DFW.h>
#include <MainRenderer/GlobalRenderThreadManager.h>

#include <D3DFramework/GraphicUtilites/RTShaderBindingTable.h>
#include <D3DFramework/Objects/ObjectVertexIndexDataCreator.h>

#include <World/World.h>
#include <System/AudioSystem.h>
#include <System/CameraSystem.h>
#include <System/LightSystem.h>
#include <System/ClusteredLightningSystem.h>
#include <System/SceneAnimationSystem.h>
#include <System/RenderMeshesSystem.h>
#include <System/RTShadowSystem.h>
#include <System/SkyboxRenderSystem.h>
#include <System/BloomEffectSystem.h>


class MainRenderer : virtual public FD3DW::D3DFW {

public:
	MainRenderer();
	virtual ~MainRenderer()=default;


	void UserInit() override;
	void UserLoop() override;
	void UserClose()  override;
	FD3DW::BaseCommandQueue* UserSwapchainCommandQueue(ID3D12Device* device) override;

	UINT GetFrameIndex();
	bool IsEnabledPreDepth();
	void EnablePreDepth(bool in);
	bool IsEnabledBloom();
	void EnableBloom(bool b);
	FLOAT* GetClearColor();

public:
	World* GetWorld();

public:
	///////////////////////////////////////////////////
	//BRIDGE METHODS

	//CAMERA
	dx::XMMATRIX GetCurrentProjectionMatrix() const;
	dx::XMMATRIX GetCurrentViewMatrix() const;
	dx::XMFLOAT3 GetCurrentCameraPosition() const;
	CameraFrustum GetCameraFrustum();

	//MESHES
	void SetMeshCullingType(MeshCullingType in);
	MeshCullingType GetMeshCullingType();
	FD3DW::AccelerationStructureBuffers GetTLAS();


	//LIGHTS
	D3D12_GPU_VIRTUAL_ADDRESS GetLightBufferConstantBufferAddress();
	D3D12_GPU_VIRTUAL_ADDRESS GetLightsStructuredBufferAddress();
	D3D12_GPU_VIRTUAL_ADDRESS GetClusterConstantBufferAddress();
	D3D12_GPU_VIRTUAL_ADDRESS GetClusterStructuredBufferAddress();
	FD3DW::StructuredBuffer* GetLightsBuffer();
	int GetLightsCount();

	//SHADOWS
	bool IsShadowEnabled();
	void SetRTShadowConfig(RTShadowSystemConfig config);
	RTShadowSystemConfig GetRTShadowConfig();

	//WORLD
	void LoadWorld(std::string pathTo);
	void SaveActiveWorld(std::string pathTo);

	//BLOOM
	BloomSystemCompositeData GetCompositeData();
	void SetCompositeData(BloomSystemCompositeData data);
	BloomSystemBrightPassData GetBrightPassData();
	void SetBrightPassData(BloomSystemBrightPassData data);
	BloomBlurType GetBloomBlurType();
	void SetBloomBlurType(BloomBlurType blurType);

protected: //TEST METHODS
	void CreateTestWorld();

protected:
	std::shared_ptr<World> CreateEmptyWorld();

private:
	void ProcessNotifiesInWorld();
	void ProcessNotifies(std::vector<NRenderSystemNotifyType> notifies);

private:
	void InitMainRendererParts(ID3D12Device* device);
	void InitMainRendererDXRParts(ID3D12Device5* device);

	void InitMainRendererSystems(ID3D12Device* device);
	void InitMainRendererDXRSystems(ID3D12Device5* device);

private:

	std::unique_ptr<MainRenderer_UIComponent> m_pUIComponent = nullptr;
	UINT m_uFrameIndex = 0;

	std::shared_ptr<World> m_pWorld;

	///////////////////////////////////////////
	std::deque<std::shared_ptr<FD3DW::ExecutionHandle>> m_dInFlight;
	size_t m_uMaxFramesInFlight = 1;

	AudioSystem* m_pAudioSystem = nullptr;
	CameraSystem* m_pCameraSystem = nullptr;
	LightSystem* m_pLightSystem = nullptr;
	ClusteredLightningSystem* m_pClusteredLightningSystem = nullptr;
	SceneAnimationSystem* m_pSceneAnimationSystem = nullptr;
	RenderMeshesSystem* m_pRenderMeshesSystem = nullptr;
	RTShadowSystem* m_pRTShadowSystem = nullptr;
	SkyboxRenderSystem* m_pSkyboxRenderSystem = nullptr;
	BloomEffectSystem* m_pBloomEffectSystem = nullptr;

	std::vector<std::unique_ptr<MainRendererComponent>> m_vSystems;

protected:

	template <typename T, typename... Args>
	std::unique_ptr<T> CreateUniqueComponent(Args&&... args) {
		auto cmp = std::make_unique<T>(std::forward<Args>(args)...);
		cmp->SetAfterConstruction(this);
		return std::move(cmp);
	}

	template <typename T, typename... Args>
	T* CreateSystem(Args&&... args) {
		auto cmp = std::make_unique<T>(std::forward<Args>(args)...);
		cmp->SetAfterConstruction(this);
		auto ptr = cmp.get();
		m_vSystems.push_back(std::move(cmp));
		return ptr;
	}

	template <typename T>
	void DestroyComponent(std::unique_ptr<T>& cmp) {
		cmp->BeforeDestruction();
		cmp = nullptr;
	}

protected:
	bool m_bIsEnabledPreDepth = false;

protected:
	std::vector<std::unique_ptr<FD3DW::RenderTarget>> m_pGBuffers;
	std::unique_ptr<FD3DW::RTVPacker> m_pGBuffersRTVPack;
	std::unique_ptr<FD3DW::SRV_UAVPacker> m_pGBuffersSRVPack;

	std::vector< std::shared_ptr<FD3DW::FResource>> m_vLCTResources;

	std::unique_ptr<FD3DW::DepthStencilView> m_pDSV;
	std::unique_ptr<FD3DW::DSVPacker> m_pDSVPack;

	std::unique_ptr<FD3DW::RenderTarget> m_pForwardRenderPassRTV;
	std::unique_ptr<FD3DW::RTVPacker> m_pForwardRenderPassRTVPack;
	std::unique_ptr<FD3DW::SRV_UAVPacker> m_pForwardRenderPassSRVPack;

	D3D12_VIEWPORT m_xSceneViewPort;
	D3D12_RECT m_xSceneRect;
	std::unique_ptr<FD3DW::ObjectVertexIndexDataCreator<FD3DW::VertexFrameWork>> m_pSceneVBV_IBV;
	std::unique_ptr<FD3DW::Rectangle> m_pScreen;
	
};

