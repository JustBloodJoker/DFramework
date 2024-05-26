#pragma once
#include "pch.h"

#include "Utilites/Macroses.h"
#include "Utilites/Structures.h"
#include "Utilites/Timer.h"

#include "Objects/SimpleObjects.h"
#include "Objects/Scene.h"

#include "GraphicUtilites/BufferMananger.h"
#include "GraphicUtilites/Shader.h"
#include "GraphicUtilites/CommandList.h"
#include "GraphicUtilites/Texture.h"
#include "GraphicUtilites/ResourcePacker.h"
#include "GraphicUtilites/PipelineStateObject.h"
#include "GraphicUtilites/RootSignature.h"
#include "GraphicUtilites/RenderTarget.h"
#include "GraphicUtilites/DepthStencilView.h"
#include "GraphicUtilites/InputLayout.h"

namespace FDW
{


	class DFW
	{
		static DFW* instance;
		static DFW* GetDFWInstance();

		static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

		DFW();

	public:

		DFW(std::wstring windowTittle, int width, int height, bool fullScreen);
		~DFW();

		virtual void __START() final;

		virtual void UserInit() = 0;
		virtual void UserLoop() = 0;
		virtual void UserClose() = 0;
		virtual void UserMouseDown(WPARAM btnState, int x, int y) = 0;
		virtual void UserMouseUp(WPARAM btnState, int x, int y) = 0;
		virtual void UserMouseMoved(WPARAM btnState, int x, int y) = 0;
		virtual void UserKeyPressed(WPARAM wParam) = 0;
		

	private:  //METHODS
		
		bool InitWindow();
		bool InitTimer();
		bool InitD3D();
		void Loop();
		void Update();
		void Release();

		void SetFullScreen();

		LRESULT CALLBACK MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	private:

		bool PAUSEWORK;

		std::unique_ptr <Timer> pTimer;

		// WINDOW
		
		std::wstring wndName;
		std::unique_ptr <WindowSettings> pWndSettings;

		//D3D12
		
		wrl::ComPtr<IDXGIFactory4> pFactory;
		wrl::ComPtr<ID3D12Fence> pFence;
		wrl::ComPtr<ID3D12CommandQueue> pCommandQueue;
		wrl::ComPtr<IDXGISwapChain> pSwapChain;
		wrl::ComPtr<ID3D12DescriptorHeap> pRTVDescriptorHeap;
		wrl::ComPtr<ID3D12DescriptorHeap> pDSVDescriptorHeap;
		wrl::ComPtr<ID3D12Resource> pSwapChainRTV[BUFFERS_COUNT];
		wrl::ComPtr<ID3D12Resource> pDepthStencilBuffer;

		D3D12_VIEWPORT mainVP;
		D3D12_RECT mainRect;

		//////////////////////////

		
		//////////////////////////

		UINT currentBackBufferIndex;

		//////////////////////////
		

		
		
		std::vector<ID3D12CommandList*> pCommandListsToExecute;

		UINT rtvDescriptorSize;
		UINT dsvDescriptorSize;
		UINT cbvsrvuavDescriptorSize;

		UINT SampleCount;
		UINT Quality;

	protected:

		const UINT Get_CBV_SRV_UAV_DescriptorSize() const;
		const UINT Get_RTV_DescriptorSize() const;
		const UINT Get_DSV_DescriptorSize() const;

		wrl::ComPtr<ID3D12Device> pDevice;


		dx::XMMATRIX mainProjectionMatrix;
		HWND hwnd;
	
		void FlushCommandQueue();

		virtual void ImmediateExecuteQueue(ID3D12CommandList** commandLists, size_t commandListsCount) final;
		virtual void ImmediateExecuteQueue(ID3D12GraphicsCommandList* commandList) final;
		virtual void PushCommandListToExecute(ID3D12GraphicsCommandList* commandList) final;
		void SetMainRenderTarget(ID3D12GraphicsCommandList* pCommandList);

		wrl::ComPtr<ID3D12GraphicsCommandList> pCommandList;
		wrl::ComPtr<ID3D12CommandAllocator> pDirectAllocator;

		//std::unique_ptr<CommandList> pcmdList;

		D3D12_CPU_DESCRIPTOR_HANDLE GetCurrBackBufferView() const noexcept;
		D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const noexcept;

		DXGI_FORMAT GetMainRTVFormat() const noexcept;

		////////////////////////////////////
		// FACADE

		std::unique_ptr<Scene> CreateScene(std::string path, bool neverUpdate);
		std::unique_ptr<Rectangle> CreateRectangle(bool neverUpdate);
		std::unique_ptr<Cube> CreateCube(bool neverUpdate);
		
		std::unique_ptr<MaterialsManager> CreateMaterialMananger();
		std::unique_ptr<Material> CreateMaterial();
		std::unique_ptr<Texture> CreateTexture(std::string path);
		
		std::unique_ptr<RenderTarget> CreateRenderTarget(const DXGI_FORMAT format, const D3D12_RTV_DIMENSION dimension, const UINT arrSize,
			const UINT width, const UINT height);

		std::unique_ptr<DepthStencilView> CreateDepthStencilView(const DXGI_FORMAT format, const D3D12_DSV_DIMENSION dimension, const UINT arrSize,
			const UINT width, const UINT height, const D3D12_DSV_FLAGS flags = D3D12_DSV_FLAG_NONE);

		template<typename T>
		std::unique_ptr<UploadBuffer<T>> CreateConstantBuffer(const size_t elementCount);

		std::unique_ptr<RTVPacker> CreateRTVPack(const UINT descriptorsCount, const UINT NodeMask = 0);
		std::unique_ptr<DSVPacker> CreateDSVPack(const UINT descriptorsCount, const UINT NodeMask = 0);
		std::unique_ptr<SRVPacker> CreateSRVPack(const UINT descriptorsCount, const UINT NodeMask = 0);
		std::unique_ptr<CBVPacker> CreateCBVPack(const UINT descriptorsCount, const UINT NodeMask = 0);
		std::unique_ptr<SamplerPacker> CreateSamplerPack(const UINT descriptorsCount, const UINT NodeMask = 0);

		std::unique_ptr<RootSingature> CreateRootSignature(CD3DX12_ROOT_PARAMETER* slotRootParameters, const UINT numParameters);

		std::unique_ptr<PipelineStateObject> CreatePSO(ID3D12RootSignature* const pRootSignature,
			const D3D12_INPUT_ELEMENT_DESC* layout, const UINT layoutSize,
			const UINT renderTargetsNum, DXGI_FORMAT rtvFormats[],
			DXGI_FORMAT dsvFormat,
			const UINT SampleMask = UINT_MAX,
			const D3D12_PRIMITIVE_TOPOLOGY_TYPE type = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, 
			ID3DBlob* vsByteCode = nullptr, ID3DBlob* psByteCode = nullptr, ID3DBlob* gsByteCode = nullptr, ID3DBlob* dsByteCode = nullptr, ID3DBlob* hsByteCode = nullptr,
			D3D12_RASTERIZER_DESC rasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
			D3D12_BLEND_DESC blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT), 
			D3D12_DEPTH_STENCIL_DESC dsvStateDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT)
			);
		//
		////////////////////////////////////
		
	};

	template<typename T>
	inline std::unique_ptr<UploadBuffer<T>> DFW::CreateConstantBuffer(const size_t elementCount)
	{
		CONSOLE_MESSAGE("DFW is creating Constant Buffer");
		return std::make_unique<UploadBuffer<T>>(pDevice.Get(), elementCount, true);
	}

}