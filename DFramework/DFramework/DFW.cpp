#include "pch.h"
#include "DFW.h"


namespace FDW
{

	DFW* DFW::instance = nullptr;

	DFW::DFW(std::wstring windowTittle, int width, int height, bool fullScreen)
	{

		if (!DFW::instance)
		{
			DFW::instance = this;
		}

		pWndSettings = std::make_unique<WindowSettings>();
		pWndSettings->fullScreen = fullScreen;
		pWndSettings->height = height;
		pWndSettings->width = width;
		wndName = windowTittle;
		
		PAUSEWORK = false;
	}

	DFW::~DFW()
	{
		if (pDevice)
			FlushCommandQueue();
		
	}

	void DFW::__START()
	{
		if (InitWindow())
		{
			CONSOLE_MESSAGE("Window created");
		}

		if (InitTimer())
		{
			CONSOLE_MESSAGE("TIMER OBJECT INITED");
		}

		if (InitD3D())
		{
			CONSOLE_MESSAGE("Render inited");
		}

		HRESULT_ASSERT(pCommandList->Reset(pDirectAllocator.Get(), nullptr), "command list reset error");

		UserInit();

		Loop();

		CONSOLE_MESSAGE("WND NOT ENABLE! END LOOP");

		Release();
	}

	D3D12_CPU_DESCRIPTOR_HANDLE DFW::GetCurrBackBufferView() const noexcept
	{
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(pRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), currentBackBufferIndex, rtvDescriptorSize);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE DFW::GetDepthStencilView() const noexcept
	{
		return pDSVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	}

	DXGI_FORMAT DFW::GetMainRTVFormat() const noexcept
	{
		return DXGI_FORMAT_R8G8B8A8_UNORM;
	}

	bool DFW::InitWindow()
	{
		CONSOLE_MESSAGE("Creating window");

		WNDCLASSEX wc = {};

		wc.cbSize = sizeof(WNDCLASSEX);
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = DFW::WndProc;
		wc.cbClsExtra = NULL;
		wc.cbWndExtra = NULL;
		wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 2);
		wc.lpszMenuName = NULL;
		wc.lpszClassName = L"wndClass";
		wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

		SAFE_ASSERT(RegisterClassEx(&wc), "Register wnd class error");

		hwnd = CreateWindowEx(NULL,
			L"wndClass",
			this->wndName.c_str(),
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT,
			this->pWndSettings->width, this->pWndSettings->height,
			NULL,
			NULL,
			NULL,
			NULL);

		SAFE_ASSERT(hwnd, "Create hwnd error");

		ShowWindow(hwnd, 1);
		UpdateWindow(hwnd);
		return true;
	}

	bool DFW::InitTimer()
	{
		pTimer = std::make_unique<Timer>();
		return pTimer ? true : false;
	}

	bool DFW::InitD3D()
	{
#if defined(_DEBUG)

		wrl::ComPtr<ID3D12Debug> pDebugController;
		HRESULT_ASSERT(D3D12GetDebugInterface(IID_PPV_ARGS(&pDebugController)), "ID3D12 Debug mode set error");

#endif

		HRESULT_ASSERT(CreateDXGIFactory1(IID_PPV_ARGS(&pFactory)), "Factory create error");

		HRESULT hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(pDevice.GetAddressOf()));
		if (FAILED(hr))
		{
			wrl::ComPtr<IDXGIAdapter> pWarpAdapter;
			HRESULT_ASSERT(pFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)), "Warp adapter error");

			HRESULT_ASSERT(D3D12CreateDevice(pWarpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(pDevice.GetAddressOf())), "Device create error");
		}

		HRESULT_ASSERT(pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(pFence.GetAddressOf())), "Fence create error");

		rtvDescriptorSize = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		cbvsrvuavDescriptorSize = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		dsvDescriptorSize = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

		//MSAA 4x
		/*D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS qualityLevels;
		qualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		qualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
		qualityLevels.NumQualityLevels = 0;
		qualityLevels.SampleCount = 4;

		HRESULT_ASSERT(pDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &qualityLevels, sizeof(qualityLevels)), "MSAAx4 check error");

		MSAA4xQualitySupport = qualityLevels.NumQualityLevels;
		HRESULT_ASSERT(MSAA4xQualitySupport > 0, "MSAA quality error");*/


		D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
		commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		HRESULT_ASSERT(pDevice->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(pCommandQueue.GetAddressOf())), "Command queue create error");

		
		HRESULT_ASSERT(pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(pDirectAllocator.GetAddressOf())),
			"Command allocator create error");

		HRESULT_ASSERT(pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, pDirectAllocator.Get(), nullptr, IID_PPV_ARGS(pCommandList.GetAddressOf())),
			"Command list create error");
		pCommandList->Close();

		DXGI_SWAP_CHAIN_DESC swapChainDesc;

		swapChainDesc.BufferDesc.Width = pWndSettings->width;
		swapChainDesc.BufferDesc.Height = pWndSettings->height;
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
		swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		swapChainDesc.SampleDesc.Count = SampleCount = 1;           //MSAA DISABLED
		swapChainDesc.SampleDesc.Quality = Quality = 0;         //MSAA DISABLED
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.OutputWindow = hwnd;
		swapChainDesc.BufferCount = BUFFERS_COUNT;
		swapChainDesc.Windowed = !pWndSettings->fullScreen;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

		HRESULT_ASSERT(pFactory->CreateSwapChain(pCommandQueue.Get(), &swapChainDesc, pSwapChain.GetAddressOf()), "Swapchain create error");

		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
		rtvHeapDesc.NumDescriptors = BUFFERS_COUNT;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		rtvHeapDesc.NodeMask = 0;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		HRESULT_ASSERT(pDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(pRTVDescriptorHeap.GetAddressOf())), "RTV descriptor heap create error");

		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
		dsvHeapDesc.NumDescriptors = 1;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		dsvHeapDesc.NodeMask = 0;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		HRESULT_ASSERT(pDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(pDSVDescriptorHeap.GetAddressOf())), "DSV descriptor heap create error");

		currentBackBufferIndex = 0;


		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeap(pRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

		for (UINT index = 0; index < BUFFERS_COUNT; index++)
		{
			HRESULT_ASSERT(pSwapChain->GetBuffer(index, IID_PPV_ARGS(pSwapChainRTV[index].GetAddressOf())), "Swapchain buffer get error");
			pDevice->CreateRenderTargetView(pSwapChainRTV[index].Get(), nullptr, rtvHeap);

			rtvHeap.Offset(1, rtvDescriptorSize);
		}

		D3D12_RESOURCE_DESC dsvDesc;
		dsvDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		dsvDesc.Alignment = 0;
		dsvDesc.Width = pWndSettings->width;
		dsvDesc.Height = pWndSettings->height;
		dsvDesc.DepthOrArraySize = 1;
		dsvDesc.MipLevels = 1;
		dsvDesc.SampleDesc.Count = 1;
		dsvDesc.SampleDesc.Quality = 0;
		dsvDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		dsvDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		CD3DX12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

		HRESULT_ASSERT(pDevice->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &dsvDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(pDepthStencilBuffer.GetAddressOf())), "Depth stencil buffer create error");

		pDevice->CreateDepthStencilView(pDepthStencilBuffer.Get(), nullptr, GetDepthStencilView());

		CD3DX12_RESOURCE_BARRIER resBar = CD3DX12_RESOURCE_BARRIER::Transition(pDepthStencilBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		pCommandList->ResourceBarrier(1, &resBar);

		mainVP.Height = static_cast<float>(pWndSettings->height);
		mainVP.Width = static_cast<float>(pWndSettings->width);
		mainVP.MaxDepth = 1.0f;
		mainVP.MinDepth = 0.0f;
		mainVP.TopLeftX = 0.0f;
		mainVP.TopLeftY = 0.0f;

		mainRect.left = 0;
		mainRect.right = pWndSettings->width;
		mainRect.top = 0;
		mainRect.bottom = pWndSettings->height;

		return true;
	}

	void DFW::Loop()
	{
		MSG msg;
		ZeroMemory(&msg, sizeof(MSG));

		while (IsWindowEnabled(hwnd))
		{
			if (PeekMessage(&msg, hwnd, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
			{
				if (!PAUSEWORK)
				{
					pTimer->Tick();
					Update();
				}
			}
		}
	}

	void DFW::Update()
	{
		CD3DX12_RESOURCE_BARRIER barrier;

		HRESULT_ASSERT(pDirectAllocator->Reset(), "Command allocator reset error");

		HRESULT_ASSERT(pCommandList->Reset(pDirectAllocator.Get(), nullptr), "Command list reset error");

		pCommandList->RSSetViewports(1, &mainVP);
		pCommandList->RSSetScissorRects(1, &mainRect);

		pCommandList->ResourceBarrier(1, &keep(CD3DX12_RESOURCE_BARRIER::Transition(pSwapChainRTV[currentBackBufferIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET)));

		static FLOAT clearColor[4] = { 0.2f,0.2f,0.2f,1.0f };

		pCommandList->ClearRenderTargetView(GetCurrBackBufferView(), clearColor, 0, nullptr);
		pCommandList->ClearDepthStencilView(GetDepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

		pCommandList->OMSetRenderTargets(1, &keep(GetCurrBackBufferView()), true, &keep(GetDepthStencilView()));
		

		/////////////////// 
		// USER DRAW
		UserLoop();
		//////////////////



		pCommandList->ResourceBarrier(1, &keep(CD3DX12_RESOURCE_BARRIER::Transition(pSwapChainRTV[currentBackBufferIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT)));
		hr = pCommandList->Close();
		HRESULT_ASSERT(hr, "Command list close error");

		pCommandListsToExecute.push_back(pCommandList.Get());

		pCommandQueue->ExecuteCommandLists(pCommandListsToExecute.size(), &pCommandListsToExecute[0]);


		HRESULT_ASSERT(pSwapChain->Present(0, 0), "Swapchain present error");

		currentBackBufferIndex = (currentBackBufferIndex + 1) % BUFFERS_COUNT;

		FlushCommandQueue();

		pCommandListsToExecute.clear();
	}

	void DFW::Release()
	{
		UserClose();
		CONSOLE_MESSAGE("USER CLOSED");

		pTimer.release();

		CONSOLE_MESSAGE("FRAMEWORK CLOSED");
	}

	void DFW::SetFullScreen()
	{
		if (!pWndSettings->fullScreen)
		{
			pWndSettings->fullScreen = true;
			HMONITOR hmon = MonitorFromWindow(hwnd,
				MONITOR_DEFAULTTONEAREST);
			MONITORINFO mi = { sizeof(mi) };
			GetMonitorInfo(hmon, &mi);
			pWndSettings->width = mi.rcMonitor.right - mi.rcMonitor.left;
			pWndSettings->height = mi.rcMonitor.bottom - mi.rcMonitor.top;
		}
	}

	LRESULT DFW::MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg)
		{
		case WM_ACTIVATE:
		{
			CONSOLE_MESSAGE_NO_PREF("WM_ACTIVATE ACTIVE");

			if (LOWORD(wParam) == WA_INACTIVE)
			{
				PAUSEWORK = true;
			}
			else
			{
				PAUSEWORK = false;
			}
			return 0;
		}

		case WM_KEYDOWN:

			CONSOLE_MESSAGE_NO_PREF(std::string("PRESSED KEY ID: " + std::to_string(lParam)));

			if (wParam == VK_ESCAPE)
			{
				CONSOLE_MESSAGE("ESCAPE PRESSED");
				DestroyWindow(hWnd);
			}
			else
			{
				UserKeyPressed(wParam);
			}
			return 0;

		case WM_DESTROY:

			CONSOLE_MESSAGE("WM_DESTROY MESSAGE ACTIVE");
			PostQuitMessage(0);
			return 0;

		case WM_SIZE:

			CONSOLE_MESSAGE("WM_SIZE ACTIVE");

			mainProjectionMatrix = dx::XMMatrixPerspectiveFovLH(M_PI_2, pWndSettings->width / pWndSettings->height, 1.0f, 10000.0f);

			SetWindowPos(hWnd, hWnd, CW_USEDEFAULT, CW_USEDEFAULT, LOWORD(lParam), HIWORD(lParam), WS_OVERLAPPEDWINDOW);

		case WM_ENTERSIZEMOVE:
		{
			CONSOLE_MESSAGE("WM_ENTERSIZE ACTIVE");

			SetWindowPos(hWnd, hWnd, CW_USEDEFAULT, CW_USEDEFAULT, LOWORD(lParam), HIWORD(lParam), WS_OVERLAPPEDWINDOW);

			return 0;
		}
		case WM_EXITSIZEMOVE:
		{
			CONSOLE_MESSAGE("WM_EXITSIZE ACTIVE");

			SetWindowPos(hWnd, hWnd, CW_USEDEFAULT, CW_USEDEFAULT, LOWORD(lParam), HIWORD(lParam), WS_OVERLAPPEDWINDOW);

			return 0;
		}

		case WM_MENUCHAR:
			return MAKELRESULT(0, MNC_CLOSE);

		case WM_GETMINMAXINFO:
			((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
			((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
			return 0;

		case WM_LBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_RBUTTONDOWN:
			UserMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			return 0;
		case WM_LBUTTONUP:
		case WM_MBUTTONUP:
		case WM_RBUTTONUP:
			UserMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			return 0;
		case WM_MOUSEMOVE:
			CONSOLE_MESSAGE_NO_PREF(std::string("MOUSE MOVED X: " + std::to_string(GET_X_LPARAM(lParam)) + " Y: " + std::to_string(GET_Y_LPARAM(lParam))));
			UserMouseMoved(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			return 0;

		}
		return DefWindowProc(hWnd,
			msg,
			wParam,
			lParam);
	}

	const UINT DFW::Get_CBV_SRV_UAV_DescriptorSize() const
	{
		return cbvsrvuavDescriptorSize;
	}

	const UINT DFW::Get_RTV_DescriptorSize() const
	{
		return rtvDescriptorSize;
	}

	const UINT DFW::Get_DSV_DescriptorSize() const
	{
		return dsvDescriptorSize;
	}

	void DFW::FlushCommandQueue()
	{
		auto nextFence = pFence->GetCompletedValue() + 1;
		HRESULT_ASSERT(pCommandQueue->Signal(pFence.Get(),
			nextFence), "Fence signal error");

		if (pFence->GetCompletedValue() < nextFence)
		{
			HANDLE eventHandle = CreateEventEx(nullptr, L"", false, EVENT_ALL_ACCESS);
			HRESULT_ASSERT(pFence->SetEventOnCompletion(nextFence, eventHandle), "Fence set event error");
			WaitForSingleObject(eventHandle, INFINITE);
			CloseHandle(eventHandle);
		}
	}

	void DFW::ImmediateExecuteQueue(ID3D12CommandList** commandLists, size_t commandListsCount)
	{
		pCommandQueue->ExecuteCommandLists(commandListsCount, commandLists);
		FlushCommandQueue();
	}

	void DFW::ImmediateExecuteQueue(ID3D12GraphicsCommandList* commandList)
	{
		ID3D12CommandList* cmdLists[] = { commandList };
		ImmediateExecuteQueue(cmdLists, 1);
	}

	void DFW::PushCommandListToExecute(ID3D12GraphicsCommandList* commandList)
	{
		auto el = std::find(pCommandListsToExecute.begin(), pCommandListsToExecute.end(), commandList);

		if (el == pCommandListsToExecute.end())
			pCommandListsToExecute.push_back(commandList);

	}

	void DFW::SetMainRenderTarget(ID3D12GraphicsCommandList* pCommandList)
	{
		pCommandList->OMSetRenderTargets(1, &keep(GetCurrBackBufferView()), true, &keep(GetDepthStencilView()));
	}


	DFW* DFW::GetDFWInstance()
	{
		return instance;
	}

	LRESULT DFW::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		return DFW::GetDFWInstance()->MsgProc(hWnd, msg, wParam, lParam);
	}

	std::unique_ptr<RenderTarget> FDW::DFW::CreateRenderTarget(const DXGI_FORMAT format, const D3D12_RTV_DIMENSION dimension, const UINT arrSize, const UINT width, const UINT height)
	{
		CONSOLE_MESSAGE("DFW is creating RTV");
		return std::make_unique<FDW::RenderTarget>(pDevice.Get(), format, dimension, arrSize, width, height, DXGI_SAMPLE_DESC({ SampleCount, Quality }));
	}

	std::unique_ptr<RTVPacker> DFW::CreateRTVPack(const UINT descriptorsCount, const UINT NodeMask)
	{
		CONSOLE_MESSAGE("DFW is creating RTV pack");
		return std::make_unique<RTVPacker>(Get_RTV_DescriptorSize(), descriptorsCount, NodeMask, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, pDevice.Get());
	}

	std::unique_ptr<DSVPacker> DFW::CreateDSVPack(const UINT descriptorsCount, const UINT NodeMask)
	{
		CONSOLE_MESSAGE("DFW is creating DSV pack");
		return std::make_unique<DSVPacker>(Get_RTV_DescriptorSize(), descriptorsCount, NodeMask, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, pDevice.Get());
	}

	std::unique_ptr<SRVPacker> DFW::CreateSRVPack(const UINT descriptorsCount, const UINT NodeMask)
	{
		CONSOLE_MESSAGE("DFW is creating SRV pack");
		return std::make_unique<SRVPacker>(Get_CBV_SRV_UAV_DescriptorSize(), descriptorsCount, NodeMask, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, pDevice.Get());
	}

	std::unique_ptr<CBVPacker> DFW::CreateCBVPack(const UINT descriptorsCount, const UINT NodeMask)
	{
		CONSOLE_MESSAGE("DFW is creating CBV pack");
		return std::make_unique<CBVPacker>(Get_CBV_SRV_UAV_DescriptorSize(), descriptorsCount, NodeMask, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, pDevice.Get());
	}

	std::unique_ptr<SamplerPacker> DFW::CreateSamplerPack(const UINT descriptorsCount, const UINT NodeMask)
	{
		CONSOLE_MESSAGE("DFW is creating Samplers pack");
		return std::make_unique<SamplerPacker>(Get_CBV_SRV_UAV_DescriptorSize(), descriptorsCount, NodeMask, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, pDevice.Get());
	}

	std::unique_ptr<RootSingature> DFW::CreateRootSignature(CD3DX12_ROOT_PARAMETER* slotRootParameters, const UINT numParameters)
	{
		CONSOLE_MESSAGE("DFW is creating Root Signature");
		std::unique_ptr<RootSingature> result = std::make_unique<RootSingature>(slotRootParameters, numParameters);
		result->CreateRootSignature(pDevice.Get());
		return std::move(result);
	}

	std::unique_ptr<PipelineStateObject> DFW::CreatePSO(ID3D12RootSignature* const pRootSignature, const D3D12_INPUT_ELEMENT_DESC* layout, const UINT layoutSize, const UINT renderTargetsNum, DXGI_FORMAT rtvFormats[], DXGI_FORMAT dsvFormat, const UINT SampleMask, const D3D12_PRIMITIVE_TOPOLOGY_TYPE type, ID3DBlob* vsByteCode, ID3DBlob* psByteCode, ID3DBlob* gsByteCode, ID3DBlob* dsByteCode, ID3DBlob* hsByteCode, D3D12_RASTERIZER_DESC rasterizerDesc, D3D12_BLEND_DESC blendDesc, D3D12_DEPTH_STENCIL_DESC dsvStateDesc)
	{
		CONSOLE_MESSAGE("DFW is creating Pipeline State");
		std::unique_ptr<PipelineStateObject> result = std::make_unique<FDW::PipelineStateObject>(pRootSignature, layout, layoutSize, renderTargetsNum, rtvFormats, dsvFormat);
		result->SetRasterizerState(rasterizerDesc);
		result->SetBlendState(blendDesc);
		result->SetDepthStencilState(dsvStateDesc);
		result->SetSampleDesc(DXGI_SAMPLE_DESC({ SampleCount, Quality }));
		
		if (vsByteCode) result->SetVS(vsByteCode);
		if (psByteCode) result->SetPS(psByteCode);
		if (gsByteCode) result->SetGS(gsByteCode);
		if (dsByteCode) result->SetDS(dsByteCode);
		if (hsByteCode) result->SetHS(hsByteCode);

		result->CreatePSO(pDevice.Get());

		return std::move(result);
	}

	std::unique_ptr<DepthStencilView> DFW::CreateDepthStencilView(const DXGI_FORMAT format, const D3D12_DSV_DIMENSION dimension, const UINT arrSize, const UINT width, const UINT height, const D3D12_DSV_FLAGS flags)
	{
		CONSOLE_MESSAGE("DFW is creating DSV");
		return std::make_unique<DepthStencilView>(pDevice.Get(), format, dimension, arrSize, width, height, DXGI_SAMPLE_DESC({ SampleCount, Quality }), flags);
	}

	std::unique_ptr<Scene> DFW::CreateScene(std::string path, bool neverUpdate)
	{
		CONSOLE_MESSAGE("DFW is creating Scene");
		return std::make_unique<Scene>(path, pDevice.Get(), pCommandList.Get(), neverUpdate);
	}

	std::unique_ptr<Rectangle> DFW::CreateRectangle(bool neverUpdate)
	{
		CONSOLE_MESSAGE("DFW is creating Rectangle");
		return std::make_unique<Rectangle>(pDevice.Get(), pCommandList.Get(), neverUpdate);
	}

	std::unique_ptr<Cube> DFW::CreateCube(bool neverUpdate)
	{
		CONSOLE_MESSAGE("DFW is creating Cube");
		return std::make_unique<Cube>(pDevice.Get(), pCommandList.Get(), neverUpdate);
	}

	std::unique_ptr<MaterialsManager> DFW::CreateMaterialMananger()
	{
		CONSOLE_MESSAGE("DFW is creating Materials Mananger");
		return std::make_unique<MaterialsManager>();
	}

	std::unique_ptr<Material> DFW::CreateMaterial()
	{
		CONSOLE_MESSAGE("DFW is creating Material");
		return std::make_unique<Material>();
	}

	std::unique_ptr<Texture> DFW::CreateTexture(std::string path)
	{
		CONSOLE_MESSAGE("DFW is creating Texture");
		return std::make_unique<Texture>(path, pDevice.Get(), pCommandList.Get());
	}

}