#pragma once


#include "DFW.h"

static std::random_device rd;
static std::mt19937 gen(rd());
static std::uniform_real_distribution<> disReal(-50.0f, 50.0f);


struct Particle
{
	Particle( ) 
	{
		position.x = disReal(gen);
		position.y = disReal(gen);
		position.z = disReal(gen);

		color.x = fabsf(disReal(gen) / 50.0f);
		color.y = fabsf(disReal(gen) / 50.0f);
		color.z = fabsf(disReal(gen) / 50.0f);

		velocity = fabsf(disReal(gen) / 5.0f);
	}

	dx::XMFLOAT3 position;
	float velocity;
	dx::XMFLOAT4 color;
};

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

	/////////////////////////////////////
	// 
	std::unique_ptr<FDW::Cube> cube;

	std::unique_ptr<FDW::DepthStencilView> dsv;
	std::unique_ptr<FDW::DSVPacker> dsvPack;

	std::unique_ptr<FDW::UploadBuffer<FDW::MatricesConstantBufferStructureFrameWork>> pMatricesBuffer;
	dx::XMMATRIX view;
	dx::XMMATRIX world;

	float camYaw;
	float camPitch;
	POINT mLastMousePos;
	dx::XMVECTOR eye;
	dx::XMVECTOR up;
	dx::XMVECTOR at;

	std::unique_ptr<FDW::PipelineStateObject> pso;
	std::unique_ptr<FDW::ComputePipelineStateObject> computePSO;
	std::unique_ptr<FDW::RootSingature> pRootSignatureCompute;
	std::unique_ptr<FDW::RootSingature> pRootSignnatureRender;

	std::unique_ptr<FDW::CommandQueue> pComputeCommandQueue;
	std::unique_ptr<FDW::CommandList> pComputeCommandList;
	ID3D12GraphicsCommandList* computepcml;

	std::unique_ptr<FDW::CommandList> pCommandList;
	ID3D12GraphicsCommandList* pcml;

	std::unique_ptr<FDW::Texture> rwBuffer;
	std::unique_ptr<FDW::UAVPacker> rwPack;
};

/*FDW::MatricesConstantBufferStructureFrameWork cbBuffer;
	std::unique_ptr<FDW::UploadBuffer<FDW::MatricesConstantBufferStructureFrameWork>> pConstantUploadBuffer;
	std::unique_ptr<FDW::UploadBuffer<FDW::MaterialFrameWork>> pMaterialConstantBuffer;
	std::unique_ptr<FDW::SamplerPacker> samplerPack;

	std::unique_ptr<FDW::RootSingature> pRootSignFirstPass;
	std::unique_ptr<FDW::RootSingature> pRootSignSecondPass;*/


	//std::vector<std::unique_ptr<FDW::SRVPacker>> srvPacks;

	/*std::unique_ptr<FDW::Scene> scene;

	std::unique_ptr<FDW::Rectangle> screenRes;*/

	/*std::unique_ptr<FDW::PipelineStateObject> pso;
	std::unique_ptr<FDW::PipelineStateObject> mainpso;

	std::unique_ptr<FDW::RenderTarget> rtvPos;
	std::unique_ptr<FDW::RenderTarget> rtvBase;
	std::unique_ptr<FDW::RenderTarget> rtvNormals;

	std::unique_ptr<FDW::RTVPacker> rtvPack;

	std::unique_ptr<FDW::SRVPacker> srvPackRTV;*/