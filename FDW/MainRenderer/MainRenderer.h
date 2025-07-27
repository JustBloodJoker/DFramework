#pragma once

#include <pch.h>
#include <MainRenderer/MainRendererComponent.h>

#include <UI/MainRenderer_UIComponent.h>
#include <D3DFramework/D3DFW.h>
#include <Camera/MainRenderer_CameraComponent.h>
#include <RenderableObjects/MainRenderer_RenderableObjectsManager.h>
#include <Lights/MainRenderer_LightsManager.h>

#include <RenderableObjects/RenderableMesh.h>

class MainRenderer : virtual public FD3DW::D3DFW {

public:
	MainRenderer();
	virtual ~MainRenderer()=default;

	void UserInit() override;
	void UserLoop() override;
	void UserClose()  override;

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

	//RENDERABLE OBJECTS MANAGER
	std::vector<BaseRenderableObject*> GetRenderableObjects() const;
	void AddScene(std::string path);
	void AddSkybox(std::string path);
	void AddAudio(std::string path);
	void AddSimplePlane();
	void RemoveObject(BaseRenderableObject* obj);
	void RemoveAllObjects();

	//LIGHTS MANAGER
	void CreateLight();
	void DeleteLight(int idx);
	const LightStruct& GetLight(int idx);
	void SetLightData(LightStruct newData, int idx);
	int GetLightsCount();

public:
	void SaveSceneToFile(std::string pathTo);
	void LoadSceneFromFile(std::string pathTo);


private:
	void InitMainRendererParts(ID3D12Device* device);


private:

	void AddToCallAfterRenderLoop(std::function<void(void)> foo);
	void CallAfterRenderLoop();
	std::vector<std::function<void(void)>> m_vCallAfterRenderLoop;

private:

	std::unique_ptr<MainRenderer_UIComponent> m_pUIComponent = nullptr;
	std::unique_ptr<MainRenderer_CameraComponent> m_pCameraComponent = nullptr;
	std::unique_ptr<MainRenderer_RenderableObjectsManager> m_pRenderableObjectsManager = nullptr;
	std::unique_ptr<MainRenderer_LightsManager> m_pLightsManager = nullptr;

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
	std::unique_ptr<FD3DW::CommandList> m_pCommandList;
	ID3D12GraphicsCommandList* m_pPCML;

protected:
	//TEST FIELDS
	std::vector<std::unique_ptr<FD3DW::RenderTarget>> m_pGBuffers;
	std::unique_ptr<FD3DW::RTVPacker> m_pGBuffersRTVPack;
	std::unique_ptr<FD3DW::SRVPacker> m_pGBuffersSRVPack;
	std::unique_ptr<FD3DW::DepthStencilView> m_pDSV;
	std::unique_ptr<FD3DW::DSVPacker> m_pDSVPack;

	std::unique_ptr<FD3DW::RenderTarget> m_pForwardRenderPassRTV;
	std::unique_ptr<FD3DW::RTVPacker> m_pForwardRenderPassRTVPack;
	std::unique_ptr<FD3DW::SRVPacker> m_pForwardRenderPassSRVPack;

	D3D12_VIEWPORT m_xSceneViewPort;
	D3D12_RECT m_xSceneRect;
	std::unique_ptr<FD3DW::Rectangle> m_pScreen;
	///////////////

};

