#pragma once
#include <DFWCombine/DFW.h>

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

	std::unique_ptr<FD3DW::Audio> music;
	

	/////////////////////////////////////
	// 
	std::unique_ptr<FD3DW::Scene> bird;

	std::unique_ptr<FD3DW::RenderTarget> rtv;
	std::unique_ptr<FD3DW::RTVPacker> rtvPack;
	std::unique_ptr<FD3DW::SRVPacker> rtvSrvPack;

	std::unique_ptr<FD3DW::DepthStencilView> dsv;
	std::unique_ptr<FD3DW::DSVPacker> dsvPack;

	std::unique_ptr<FD3DW::UploadBuffer<FD3DW::MatricesConstantBufferStructureFrameWork>> pMatricesBuffer;
	dx::XMMATRIX view;
	dx::XMMATRIX world;

	////////////
	// DLSS / AA
	
	D3D12_VIEWPORT sceneViewPort;
	D3D12_RECT sceneRect;

	std::unique_ptr<FD3DW::Rectangle> screen;
	std::unique_ptr<FD3DW::RootSingature> rootScreen;
	std::unique_ptr<FD3DW::PipelineStateObject> psoScreen;

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

	std::unique_ptr<FD3DW::PipelineStateObject> pso;
	std::unique_ptr<FD3DW::RootSingature> pRootSignnatureRender;

	std::unique_ptr<FD3DW::CommandList> pCommandList;
	ID3D12GraphicsCommandList* pcml;

	FDWWIN::Timer* timer;

	std::unique_ptr<FD3DW::SamplerPacker> samplerPack;
	std::vector<std::unique_ptr<FD3DW::SRVPacker>> srvPacks;

	std::unique_ptr<FD3DW::FResource> structureBufferBones;
};