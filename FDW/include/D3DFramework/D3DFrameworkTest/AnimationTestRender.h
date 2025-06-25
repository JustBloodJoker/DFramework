#pragma once
#include "D3DFramework/D3DFW.h"

class AnimationTestRender
	: public FD3DW::D3DFW
{

public:

	AnimationTestRender();

	virtual ~AnimationTestRender();

	void UserInit() override;
	void UserLoop() override;
	void UserClose()  override;
	void UserMouseDown(WPARAM btnState, int x, int y)  override;
	void UserMouseUp(WPARAM btnState, int x, int y) override;
	void UserMouseMoved(WPARAM btnState, int x, int y) override;
	void UserKeyPressed(WPARAM wParam)  override;
	void UserResizeUpdate()				override;
	void UserEndResizeUpdate()			override;

private:

	/////////////////////////////////////
	// 
	std::unique_ptr<FD3DW::Scene> m_pBird;

	std::unique_ptr<FD3DW::RenderTarget> m_pRTV;
	std::unique_ptr<FD3DW::RTVPacker> m_pRTVPack;
	std::unique_ptr<FD3DW::SRVPacker> m_pRTV_SRVPack;

	std::unique_ptr<FD3DW::DepthStencilView> m_pDSV;
	std::unique_ptr<FD3DW::DSVPacker> m_pDSVPack;

	std::unique_ptr<FD3DW::UploadBuffer<FD3DW::MatricesConstantBufferStructureFrameWork>> m_pMatricesBuffer;
	dx::XMMATRIX m_xView;
	dx::XMMATRIX m_xWorld;
	dx::XMMATRIX m_xProjectionMatrix;

	D3D12_VIEWPORT m_xSceneViewPort;
	D3D12_RECT m_xSceneRect;

	std::unique_ptr<FD3DW::Rectangle> m_pScreen;
	std::unique_ptr<FD3DW::RootSingature> m_pRootScreen;
	std::unique_ptr<FD3DW::PipelineStateObject> m_pPSOScreen;

	float m_fCamYaw;
	float m_fCamPitch;
	float m_fCamRoll;
	POINT m_xLastMousePos;
	dx::XMVECTOR m_xEye;
	dx::XMVECTOR m_xUp;
	dx::XMVECTOR m_xStartUp;
	dx::XMVECTOR m_xAt;

	std::unique_ptr<FD3DW::PipelineStateObject> m_pPSO;
	std::unique_ptr<FD3DW::RootSingature> m_pRootSignnatureRender;

	std::unique_ptr<FD3DW::CommandList> m_pCommandList;
	ID3D12GraphicsCommandList* m_pPCML;

	FDWWIN::Timer* m_pTimer;

	std::unique_ptr<FD3DW::SamplerPacker> m_pSamplerPack;
	std::vector<std::unique_ptr<FD3DW::SRVPacker>> m_vSRVPacks;

	std::unique_ptr<FD3DW::FResource> m_pStructureBufferBones;
};