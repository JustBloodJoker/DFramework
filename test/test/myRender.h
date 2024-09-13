#pragma once
#include <DFW.h>
#include <GraphicUtilites/PostProcessing.h>
#include <Utilites/AudioMananger.h>

class myRender 
	: public FDW::DFW
{

public:

	myRender();

	virtual ~myRender();

	void UserInit() override;
	void UserLoop() override;
	void UserClose()  override;
	void UserMouseDown(WPARAM btnState, int x, int y)  override;
	void UserMouseUp(WPARAM btnState, int x, int y) override;
	void UserMouseMoved(WPARAM btnState, int x, int y) override;
	void UserKeyPressed(WPARAM wParam)  override;
	void UserResizeUpdate()				override;

private:

	std::unique_ptr<FDW::Audio> music;
	

	/////////////////////////////////////
	// 
	std::unique_ptr<FDW::Scene> bird;

	std::unique_ptr<FDW::RenderTarget> rtv;
	std::unique_ptr<FDW::RTVPacker> rtvPack;
	std::unique_ptr<FDW::SRVPacker> rtvSrvPack;

	std::unique_ptr<FDW::DepthStencilView> dsv;
	std::unique_ptr<FDW::DSVPacker> dsvPack;

	std::unique_ptr<FDW::UploadBuffer<FDW::MatricesConstantBufferStructureFrameWork>> pMatricesBuffer;
	dx::XMMATRIX view;
	dx::XMMATRIX world;

	////////////
	// DLSS / AA
	
	D3D12_VIEWPORT sceneViewPort;
	D3D12_RECT sceneRect;

	std::unique_ptr<FDW::Rectangle> screen;
	std::unique_ptr<FDW::RootSingature> rootScreen;
	std::unique_ptr<FDW::PipelineStateObject> psoScreen;

	//
	////////////

	float camYaw;
	float camPitch;
	float camRoll;
	POINT mLastMousePos;
	dx::XMVECTOR eye;
	dx::XMVECTOR up;
	dx::XMVECTOR startUp;
	dx::XMVECTOR at;

	std::unique_ptr<FDW::PipelineStateObject> pso;
	std::unique_ptr<FDW::RootSingature> pRootSignnatureRender;

	std::unique_ptr<FDW::CommandList> pCommandList;
	ID3D12GraphicsCommandList* pcml;

	FDW::Timer* timer;

	std::unique_ptr<FDW::SamplerPacker> samplerPack;
	std::vector<std::unique_ptr<FDW::SRVPacker>> srvPacks;

	std::unique_ptr<FDW::Texture> structureBufferBones;
};