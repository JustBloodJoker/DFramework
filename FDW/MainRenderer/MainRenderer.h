#pragma once

#include <pch.h>
#include <MainRenderer/MainRendererComponent.h>

#include <UI/MainRenderer_UIComponent.h>
#include <D3DFramework/D3DFW.h>
#include <Camera/MainRenderer_CameraComponent.h>
#include <RenderableObjects/MainRenderer_RenderableObjectsManager.h>
#include <Lights/MainRenderer_LightsManager.h>
#include <MainRenderer/GlobalRenderThreadManager.h>

#include <RenderableObjects/RenderableMesh.h>
#include <D3DFramework/GraphicUtilites/RTShaderBindingTable.h>
#include <Lights/MainRenderer_ShadowsComponent.h>
#include <Lights/ShadowsStructures.h>
#include <Lights/MainRenderer_RTSoftShadowsComponent.h>

class MainRenderer : virtual public FD3DW::D3DFW {

public:
	MainRenderer();
	virtual ~MainRenderer()=default;

	void UserInit() override;
	void UserLoop() override;
	void UserClose()  override;
	FD3DW::BaseCommandQueue* UserSwapchainCommandQueue(ID3D12Device* device) override;

	FD3DW::DepthStencilView* GetDepthResource();
	bool IsEnabledPreDepth();
	void EnablePreDepth(bool in);

public:
	///////////////////////////////////////////////////
	//BRIDGE METHODS

	//CAMERA COMPONENT
	dx::XMMATRIX GetCurrentProjectionMatrix() const;
	dx::XMMATRIX GetCurrentViewMatrix() const;
	dx::XMFLOAT3 GetCurrentCameraPosition() const;
	float GetCameraSpeed();
	void SetCameraSpeed(float speed);
	void SetDefaultPosition();
	CameraFrustum GetCameraFrustum();

	//RENDERABLE OBJECTS MANAGER
	std::vector<BaseRenderableObject*> GetRenderableObjects() const;
	void AddScene(std::string path);
	void AddSkybox(std::string path);
	void AddAudio(std::string path);
	void AddSimplePlane();
	void AddSimpleCone();
	void AddSimpleCube();
	void AddSimpleSphere();
	void RemoveObject(BaseRenderableObject* obj);
	void RemoveAllObjects();
	void SetMeshCullingType(CullingType in);
	CullingType GetMeshCullingType();
	FD3DW::AccelerationStructureBuffers GetTLAS(ID3D12GraphicsCommandList4* list);


	//LIGHTS MANAGER
	void CreateLight();
	void DeleteLight(int idx);
	const LightStruct& GetLight(int idx);
	void SetLightData(LightStruct newData, int idx);
	int GetLightsCount();
	void BindLightConstantBuffer(UINT cbSlot, UINT rootSRVSlot, ID3D12GraphicsCommandList* list, bool IsCompute);
	

	//SHADOWS COMPONENT
	ShadowType CurrentShadowType();
	void SetRTShadowConfig(RTSoftShadowConfig config);
	RTSoftShadowConfig GetRTShadowConfig();

public:
	void SaveSceneToFile(std::string pathTo);
	void LoadSceneFromFile(std::string pathTo);


private:
	void InitMainRendererComponents();
	void InitMainRendererParts(ID3D12Device* device);
	void InitMainRendererDXRParts(ID3D12Device5* device);
	void TryInitShadowComponent();
	void CustomAfterInitShadowComponent(MainRenderer_ShadowsComponent* shadow);

private:

	void AddToCallAfterRenderLoop(std::function<void(void)> foo);
	void CallAfterRenderLoop();
	std::vector<std::function<void(void)>> m_vCallAfterRenderLoop;

	template<typename Func>
	void ScheduleCreation(Func&& func, bool deferToRenderLoop = true) {
		if (deferToRenderLoop) {
			AddToCallAfterRenderLoop([this, func = std::move(func)]() mutable {
				std::shared_ptr<FD3DW::ICommandRecipe> recipe;

				if (IsRTSupported())
				{
					recipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList4>>(D3D12_COMMAND_LIST_TYPE_DIRECT, [this, func](ID3D12GraphicsCommandList4* list) {
						func(list, list);
					});
				}
				else
				{
					recipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_DIRECT, [this, func](ID3D12GraphicsCommandList* list) {
						func(list, nullptr);
					});
				}

				auto h = GlobalRenderThreadManager::GetInstance()->Submit(recipe);
			});
		}
		else {
			std::shared_ptr<FD3DW::ICommandRecipe> recipe;

			if (IsRTSupported())
			{
				recipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList4>>(D3D12_COMMAND_LIST_TYPE_DIRECT, [this, func](ID3D12GraphicsCommandList4* list) {
					func(list, list);
				});
			}
			else
			{
				recipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_DIRECT, [this, func](ID3D12GraphicsCommandList* list) {
					func(list, nullptr);
				});
			}

			auto h = GlobalRenderThreadManager::GetInstance()->Submit(recipe);
		}
	}

private:

	std::unique_ptr<MainRenderer_UIComponent> m_pUIComponent = nullptr;
	std::unique_ptr<MainRenderer_CameraComponent> m_pCameraComponent = nullptr;
	std::unique_ptr<MainRenderer_RenderableObjectsManager> m_pRenderableObjectsManager = nullptr;
	std::unique_ptr<MainRenderer_LightsManager> m_pLightsManager = nullptr;
	std::unique_ptr<MainRenderer_ShadowsComponent> m_pShadowsComponent = nullptr;
	std::deque<std::shared_ptr<FD3DW::ExecutionHandle>> m_inFlight;
	size_t m_uMaxFramesInFlight = 5;

protected:

	template <typename T, typename... Args>
	std::unique_ptr<T> CreateUniqueComponent(Args&&... args) {
		auto cmp = std::make_unique<T>(std::forward<Args>(args)...);
		cmp->SetAfterConstruction(this);
		return std::move(cmp);
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

	std::unique_ptr<FD3DW::DepthStencilView> m_pDSV;
	std::unique_ptr<FD3DW::DSVPacker> m_pDSVPack;

	std::unique_ptr<FD3DW::RenderTarget> m_pForwardRenderPassRTV;
	std::unique_ptr<FD3DW::RTVPacker> m_pForwardRenderPassRTVPack;
	std::unique_ptr<FD3DW::SRV_UAVPacker> m_pForwardRenderPassSRVPack;

	D3D12_VIEWPORT m_xSceneViewPort;
	D3D12_RECT m_xSceneRect;
	std::unique_ptr<FD3DW::Rectangle> m_pScreen;
	
};

