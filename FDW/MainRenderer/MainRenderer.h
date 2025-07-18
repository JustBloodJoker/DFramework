#pragma once

#include <pch.h>
#include <MainRenderer/MainRendererComponent.h>

#include <UI/MainRenderer_UIComponent.h>
#include <D3DFramework/D3DFW.h>
#include <Camera/MainRenderer_CameraComponent.h>
#include <RenderableObjects/MainRenderer_RenderableObjectsManager.h>

#include <RenderableObjects/RenderableMesh.h>

class MainRenderer : virtual public FD3DW::D3DFW {

public:
	MainRenderer();
	virtual ~MainRenderer()=default;

	void UserInit() override;
	void UserLoop() override;
	void UserClose()  override;

	std::shared_ptr<FD3DW::FResource> ddsTest;
public:
	///////////////////////////////////////////////////
	//BRIDGE METHODS

	//CAMERA COMPONENT
	dx::XMMATRIX GetCurrentProjectionMatrix() const;
	dx::XMMATRIX GetCurrentViewMatrix() const;
	dx::XMFLOAT3 GetCurrentCameraPosition() const;

	//RENDERABLE OBJECTS MANAGER
	std::vector<BaseRenderableObject*> GetRenderableObjects() const;
	void AddScene(std::string path);
	void AddSkybox(std::string path);
	void AddAudio(std::string path);
	void RemoveObject(BaseRenderableObject* obj);
	void RemoveAllObjects();

private:
	void InitMainRendererParts(ID3D12Device* device);


private:

	MainRenderer_UIComponent* m_pUIComponent = nullptr;
	MainRenderer_CameraComponent* m_pCameraComponent = nullptr;
	MainRenderer_RenderableObjectsManager* m_pRenderableObjectsManager = nullptr;

protected:

	template <typename T, typename... Args>
	T* CreateComponent(Args&&... args) {
		auto cmp = std::make_unique<T>(this, std::forward<Args>(args)...);
		auto cmpRaw = cmp.get();
		this->m_vComponents.push_back(std::move(cmp));
		cmpRaw->AfterConstruction();
		return cmpRaw;
	}

	void DestroyComponent(MainRendererComponent* cmp);

protected:
	std::vector<std::unique_ptr<MainRendererComponent>> m_vComponents;

	std::unique_ptr<FD3DW::CommandList> m_pCommandList;
	ID3D12GraphicsCommandList* m_pPCML;

protected:
	//TEST FIELDS
	std::unique_ptr<FD3DW::RenderTarget> m_pRTV;
	std::unique_ptr<FD3DW::RTVPacker> m_pRTVPack;
	std::unique_ptr<FD3DW::SRVPacker> m_pRTV_SRVPack;
	std::unique_ptr<FD3DW::DepthStencilView> m_pDSV;
	std::unique_ptr<FD3DW::DSVPacker> m_pDSVPack;
	D3D12_VIEWPORT m_xSceneViewPort;
	D3D12_RECT m_xSceneRect;
	std::unique_ptr<FD3DW::Rectangle> m_pScreen;
	///////////////

};

