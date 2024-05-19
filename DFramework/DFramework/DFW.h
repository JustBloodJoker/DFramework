#pragma once
#include "pch.h"

#include "Utilites/Macroses.h"
#include "Utilites/Structures.h"
#include "Utilites/Timer.h"

#include "GraphicUtilites/BufferMananger.h"
#include "GraphicUtilites/Shader.h"



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
		
		D3D12_CPU_DESCRIPTOR_HANDLE GetCurrBackBufferView() const noexcept;
		D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const noexcept;
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
		

		UINT MSAA4xQualitySupport;
		
		std::vector<ID3D12CommandList*> pCommandListsToExecute;

	protected:

		UINT rtvDescriptorSize;
		UINT dsvDescriptorSize;
		UINT cbvsrvuavDescriptorSize;

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

	};


}