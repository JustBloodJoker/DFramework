#pragma once

#include <pch.h>
#include <MainRenderer/MainRendererComponent.h>

#include <UI/MainRenderer_UIComponent.h>
#include <D3DFramework/D3DFW.h>
#include <Camera/MainRenderer_CameraComponent.h>

class MainRenderer : virtual public FD3DW::D3DFW {

public:
	MainRenderer();
	virtual ~MainRenderer()=default;

	void UserInit() override;
	void UserLoop() override;
	void UserClose()  override;

private:


	MainRenderer_UIComponent* m_pUIComponent = nullptr;
	MainRenderer_CameraComponent* m_pCameraComponent = nullptr;


protected:

	template <typename T>
	T* CreateComponent() {
		auto cmp = std::make_unique<T>(this);
		auto cmpRaw = cmp.get();
		this->m_vComponents.push_back( std::move(cmp) );
		cmpRaw->AfterConstruction();
		return cmpRaw;
	}

	void DestroyComponent(MainRendererComponent* cmp);

protected:
	std::vector<std::unique_ptr<MainRendererComponent>> m_vComponents;

	std::unique_ptr<FD3DW::PipelineObject> m_pPSODefferedFirstPass;
	std::unique_ptr<FD3DW::PipelineObject> m_pPSODefferedSecondPass;


private:
	//Refactor these fields

	std::unique_ptr<FD3DW::Audio> m_pMusic;

	/////////////////////////////////////
	// 
	std::unique_ptr<FD3DW::Scene> m_pBird;

	std::unique_ptr<FD3DW::RenderTarget> m_pRTV;
	std::unique_ptr<FD3DW::RTVPacker> m_pRTVPack;
	std::unique_ptr<FD3DW::SRVPacker> m_pRTV_SRVPack;

	std::unique_ptr<FD3DW::DepthStencilView> m_pDSV;
	std::unique_ptr<FD3DW::DSVPacker> m_pDSVPack;

	std::unique_ptr<FD3DW::UploadBuffer<FD3DW::MatricesConstantBufferStructureFrameWork>> m_pMatricesBuffer;
	dx::XMMATRIX m_xWorld;

	D3D12_VIEWPORT m_xSceneViewPort;
	D3D12_RECT m_xSceneRect;

	std::unique_ptr<FD3DW::Rectangle> m_pScreen;

	std::unique_ptr<FD3DW::CommandList> m_pCommandList;
	ID3D12GraphicsCommandList* m_pPCML;

	FDWWIN::Timer* m_pTimer;

	std::unique_ptr<FD3DW::SamplerPacker> m_pSamplerPack;
	std::vector<std::unique_ptr<FD3DW::SRVPacker>> m_vSRVPacks;

	std::unique_ptr<FD3DW::FResource> m_pStructureBufferBones;


};

