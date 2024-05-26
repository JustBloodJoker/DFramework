#pragma once


#include "DFW.h"


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

private:

	FDW::MatricesConstantBufferStructureFrameWork cbBuffer;
	std::unique_ptr<FDW::UploadBuffer<FDW::MatricesConstantBufferStructureFrameWork>> pConstantUploadBuffer;
	std::unique_ptr<FDW::UploadBuffer<FDW::MaterialFrameWork>> pMaterialConstantBuffer;
	std::unique_ptr<FDW::SamplerPacker> samplerPack;

	std::unique_ptr<FDW::RootSingature> pRootSignFirstPass;
	std::unique_ptr<FDW::RootSingature> pRootSignSecondPass;

//	std::unique_ptr<FDW::SRVPacker> srvPack;

	std::vector<std::unique_ptr<FDW::SRVPacker>> srvPacks;

	dx::XMMATRIX view;
	dx::XMMATRIX world;

	float camYaw;
	float camPitch;
	POINT mLastMousePos;
	dx::XMVECTOR eye;
	dx::XMVECTOR up;
	dx::XMVECTOR at;

	std::unique_ptr<FDW::Scene> scene;

	std::unique_ptr<FDW::Rectangle> screenRes;

	std::unique_ptr<FDW::PipelineStateObject> pso;
	std::unique_ptr<FDW::PipelineStateObject> mainpso;

	std::unique_ptr<FDW::RenderTarget> rtvPos;
	std::unique_ptr<FDW::RenderTarget> rtvBase;
	std::unique_ptr<FDW::RenderTarget> rtvNormals;
	std::unique_ptr<FDW::DepthStencilView> dsv;

	std::unique_ptr<FDW::DSVPacker> dsvPack;
	std::unique_ptr<FDW::RTVPacker> rtvPack;
	std::unique_ptr<FDW::SRVPacker> srvPackRTV;
};

