#include "pch.h"
#include "D3DFW.h"


namespace FD3DW
{
	D3D12_CPU_DESCRIPTOR_HANDLE D3DFW::GetCurrBackBufferView() const noexcept
	{
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_pRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_uCurrentBackBufferIndex, m_uRTVDescriptorSize);
	}

	ID3D12Resource* D3DFW::GetCurrBackBufferResource() const noexcept
	{
		return m_aSwapChainRTV[m_uCurrentBackBufferIndex].Get();
	}


	DXGI_FORMAT D3DFW::GetMainRTVFormat() const noexcept
	{
		return DXGI_FORMAT_R8G8B8A8_UNORM;
	}

	AudioManager* D3DFW::GetAudioMananger() const noexcept
	{
		return m_pAudioMananger.get();
	}

	ID3D12CommandQueue* D3DFW::GetCommandQueue() const noexcept
	{
		return m_pCommandQueue->GetQueue();
	}

	ID3D12GraphicsCommandList* D3DFW::GetBindedCommandList() const noexcept 
	{
		return m_pBindedMainCommandList->GetPtrCommandList();
	}


	IDXGISwapChain* D3DFW::GetSwapChain() const noexcept 
	{
		return m_pSwapChain.Get();
	}

	wrl::ComPtr<IDXGISwapChain> D3DFW::GetComPtrSwapChain() const noexcept
	{
		return m_pSwapChain;
	}

	UINT D3DFW::GetCurrentBackBufferIndex() const noexcept
	{
		return m_uCurrentBackBufferIndex;
	}

	void D3DFW::PresentSwapchain()
	{
		hr = m_pSwapChain->Present(UINT(m_bVSync), 0);
		if (FAILED(hr)) {
			auto hr_device = m_pDevice->GetDeviceRemovedReason();
			if (FAILED(hr_device)) {
				hr = hr_device;
			}

			char buf[512];
			FormatMessageA(
				FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				nullptr,
				hr,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				buf,
				sizeof(buf),
				nullptr
			);
			OutputDebugStringA(buf);

			HRESULT_ASSERT(hr, "Swapchain present error");
		}

		m_uCurrentBackBufferIndex = (m_uCurrentBackBufferIndex + 1) % BUFFERS_COUNT;
	}

	void D3DFW::BeginDraw(ID3D12GraphicsCommandList* pCommandList)
	{
		pCommandList->ResourceBarrier(1, &keep(CD3DX12_RESOURCE_BARRIER::Transition(m_aSwapChainRTV[m_uCurrentBackBufferIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET)));
	}

	void D3DFW::EndDraw(ID3D12GraphicsCommandList* pCommandList)
	{
		pCommandList->ResourceBarrier(1, &keep(CD3DX12_RESOURCE_BARRIER::Transition(m_aSwapChainRTV[m_uCurrentBackBufferIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT)));
	}

	void D3DFW::BindMainViewPort(ID3D12GraphicsCommandList* pCommandList)
	{
		pCommandList->RSSetViewports(1, &m_xMainVP);
	}

	void D3DFW::BindMainRect(ID3D12GraphicsCommandList* pCommandList)
	{
		pCommandList->RSSetScissorRects(1, &m_xMainRect);
	}

	void D3DFW::BindMainCommandList(CommandList* pCommandList)
	{
		if (m_pBindedMainCommandList) 
		{
			UnbindListFromMainQueue(m_pBindedMainCommandList);
		}

		m_pBindedMainCommandList = pCommandList;
		BindListToMainQueue(m_pBindedMainCommandList);
	}

	void D3DFW::BindListToMainQueue(CommandList* pCommandList)
	{
		m_pCommandQueue->BindCommandList(pCommandList);
	}

	void D3DFW::UnbindListFromMainQueue(CommandList* pCommandList)
	{
		m_pCommandQueue->UnbindCommandList(pCommandList);
	}

	void D3DFW::ExecuteMainQueue()
	{
		m_pCommandQueue->ExecuteQueue(true);
	}

	void D3DFW::SetVSync(bool enable)
	{
		if (enable) 
		{
			CONSOLE_MESSAGE_NO_PREF("VSYNC ENABLED");
		}
		else
		{
			CONSOLE_MESSAGE_NO_PREF("VSYNC DISABLED");
		}
		m_bVSync = enable;
	}

	UINT D3DFW::GetMSAAQualitySupport(const UINT msaaSamples) const
	{
		D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS qualityLevels;
		qualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		qualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
		qualityLevels.NumQualityLevels = 0;
		qualityLevels.SampleCount = msaaSamples;

		HRESULT_ASSERT(m_pDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &qualityLevels, sizeof(qualityLevels)), "MSAA check quality error");

		if (!qualityLevels.NumQualityLevels) 
			CONSOLE_ERROR_MESSAGE("INCORRECT QUALITY MSAA LEVEL! CHECK INPUTS");

		return qualityLevels.NumQualityLevels;
	}

	void D3DFW::ChildLoop()
	{
		Update();
	}

	bool D3DFW::ChildInit()
	{
		
		if (!InitAudioMananger())
		{
			return false;
		}
		CONSOLE_MESSAGE("AUDIO MANANGER OBJECT INITED");

		if (!InitD3D())
		{
			return false;
		}
		CONSOLE_MESSAGE("Render inited");

		UserInit();
		
		AddToRouter(GetInputRouter());
		
		return true;
	}

	void D3DFW::ChildRelease()
	{
		UserClose();
		CONSOLE_MESSAGE("USER CLOSED");

		CONSOLE_MESSAGE("FRAMEWORK CLOSED");
	}

	void D3DFW::CallBeforePresent() {}

	void D3DFW::CallAfterPresent() {}

	bool D3DFW::InitAudioMananger()
	{
		m_pAudioMananger = std::make_unique<AudioManager>();
		return m_pAudioMananger ? true : false;
	}

	bool D3DFW::InitD3D()
	{
#if defined(_DEBUG)

		wrl::ComPtr<ID3D12Debug> pDebugController;
		HRESULT_ASSERT(D3D12GetDebugInterface(IID_PPV_ARGS(&pDebugController)), "ID3D12 Debug mode set error");

#endif

		HRESULT_ASSERT(CreateDXGIFactory1(IID_PPV_ARGS(&m_pFactory)), "Factory create error");

		HRESULT hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(m_pDevice.GetAddressOf()));
		if (FAILED(hr))
		{
			wrl::ComPtr<IDXGIAdapter> pWarpAdapter;
			HRESULT_ASSERT(m_pFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)), "Warp adapter error");

			HRESULT_ASSERT(D3D12CreateDevice(pWarpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(m_pDevice.GetAddressOf())), "Device create error");
		}
		m_uRTVDescriptorSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		m_uCBV_SRV_UAVDescriptorSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_uDSVDescriptorSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

		m_pCommandQueue = CreateQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);

		const auto& WndSet = WNDSettings();

		DXGI_SWAP_CHAIN_DESC swapChainDesc;
		swapChainDesc.BufferDesc.Width = WndSet.Width;
		swapChainDesc.BufferDesc.Height = WndSet.Height;
		swapChainDesc.BufferDesc.Format = GetMainRTVFormat();
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
		swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		swapChainDesc.SampleDesc.Count = m_uSampleCount = 1;           //MSAA DISABLED
		swapChainDesc.SampleDesc.Quality = m_uQuality = 0;         //MSAA DISABLED
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.OutputWindow = GETHWND();
		swapChainDesc.BufferCount = BUFFERS_COUNT;
		swapChainDesc.Windowed = !WndSet.FullScreen;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

		HRESULT_ASSERT(m_pFactory->CreateSwapChain(m_pCommandQueue->GetQueue(), &swapChainDesc, m_pSwapChain.GetAddressOf()), "Swapchain create error");

		ClearSwapchainData();
		CreateSwapchainData();

		SetViewportData(WndSet.Width, WndSet.Height);
		SetRectData(WndSet.Width, WndSet.Height);
		
		return true;
	}

	
	void D3DFW::Update()
	{
		if (!m_pBindedMainCommandList) {
			CONSOLE_ERROR_MESSAGE(" Main command list not binded. Check code and call BindMainCommandList() ");
			return;
		}

		BeginDraw(m_pBindedMainCommandList->GetPtrCommandList());

		/////////////////// 
		// USER DRAW
		UserLoop();
		//////////////////

		CallBeforePresent();

		EndDraw(m_pBindedMainCommandList->GetPtrCommandList());
		
		ExecuteMainQueue();

		PresentSwapchain();
	
		CallAfterPresent();
	}

	bool D3DFW::ProcessInput(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		if (msg == WM_EXITSIZEMOVE) {
			const auto& WndSet = WNDSettings();

			m_pCommandQueue->FlushQueue();

			ClearSwapchainData();

			hr = m_pSwapChain->ResizeBuffers(BUFFERS_COUNT, UINT(WndSet.Width), UINT(WndSet.Height), GetMainRTVFormat(), DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
			m_xMainVP.Height = static_cast<float>(WndSet.Height);
			m_xMainVP.Width = static_cast<float>(WndSet.Width);

			m_xMainRect.left = 0;
			m_xMainRect.right = WndSet.Width;
			m_xMainRect.top = 0;
			m_xMainRect.bottom = WndSet.Height;

			CreateSwapchainData();
		}

		return false;
	}

	const UINT D3DFW::Get_CBV_SRV_UAV_DescriptorSize() const noexcept
	{
		return m_uCBV_SRV_UAVDescriptorSize;
	}

	const UINT D3DFW::Get_RTV_DescriptorSize() const noexcept
	{
		return m_uRTVDescriptorSize;
	}

	const UINT D3DFW::Get_DSV_DescriptorSize() const noexcept
	{
		return m_uDSVDescriptorSize;
	}

	ID3D12Device* D3DFW::GetDevice() const noexcept
	{
		return m_pDevice.Get();
	}

	wrl::ComPtr<ID3D12Device> D3DFW::GetComPtrDevice() const noexcept
	{
		return m_pDevice;
	}

	WindowSettings D3DFW::GetMainWNDSettings() const noexcept
	{
		return WNDSettings();
	}

	D3D12_VIEWPORT D3DFW::GetMainViewPort() const noexcept
	{
		return m_xMainVP;
	}

	D3D12_RECT D3DFW::GetMainRect() const noexcept
	{
		return m_xMainRect;
	}

	std::unique_ptr<RenderTarget> D3DFW::CreateRenderTarget(const DXGI_FORMAT format, const D3D12_RTV_DIMENSION dimension, const UINT arrSize, const UINT width, const UINT height, const UINT msaaSampleCount)
	{
		CONSOLE_MESSAGE("D3DFW is creating RTV");
		return std::make_unique<RenderTarget>(m_pDevice.Get(), format, dimension, arrSize, width, height, DXGI_SAMPLE_DESC({ msaaSampleCount, m_uQuality }));
	}

	std::unique_ptr<RTVPacker> D3DFW::CreateRTVPack(const UINT descriptorsCount, const UINT NodeMask)
	{
		CONSOLE_MESSAGE("D3DFW is creating RTV pack");
		return std::make_unique<RTVPacker>(Get_RTV_DescriptorSize(), descriptorsCount, NodeMask, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, m_pDevice.Get());
	}

	std::unique_ptr<DSVPacker> D3DFW::CreateDSVPack(const UINT descriptorsCount, const UINT NodeMask)
	{
		CONSOLE_MESSAGE("D3DFW is creating DSV pack");
		return std::make_unique<DSVPacker>(Get_DSV_DescriptorSize(), descriptorsCount, NodeMask, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, m_pDevice.Get());
	}

	std::unique_ptr<SRVPacker> D3DFW::CreateSRVPack(const UINT descriptorsCount, const UINT NodeMask)
	{
		CONSOLE_MESSAGE("D3DFW is creating SRV pack");
		return std::make_unique<SRVPacker>(Get_CBV_SRV_UAV_DescriptorSize(), descriptorsCount, NodeMask, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, m_pDevice.Get());
	}

	std::unique_ptr<CBVPacker> D3DFW::CreateCBVPack(const UINT descriptorsCount, const UINT NodeMask)
	{
		CONSOLE_MESSAGE("D3DFW is creating CBV pack");
		return std::make_unique<CBVPacker>(Get_CBV_SRV_UAV_DescriptorSize(), descriptorsCount, NodeMask, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, m_pDevice.Get());
	}

	std::unique_ptr<UAVPacker> D3DFW::CreateUAVPack(const UINT descriptorsCount, const UINT NodeMask)
	{
		CONSOLE_MESSAGE("D3DFW is creating UAV pack");
		return std::make_unique<UAVPacker>(Get_CBV_SRV_UAV_DescriptorSize(), descriptorsCount, NodeMask, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, m_pDevice.Get());
	}

	std::unique_ptr<SamplerPacker> D3DFW::CreateSamplerPack(const UINT descriptorsCount, const UINT NodeMask)
	{
		CONSOLE_MESSAGE("D3DFW is creating Samplers pack");
		return std::make_unique<SamplerPacker>(Get_CBV_SRV_UAV_DescriptorSize(), descriptorsCount, NodeMask, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, m_pDevice.Get());
	}

	std::unique_ptr<RootSingature> D3DFW::CreateRootSignature(CD3DX12_ROOT_PARAMETER* slotRootParameters, const UINT numParameters)
	{
		CONSOLE_MESSAGE("D3DFW is creating Root Signature");
		std::unique_ptr<RootSingature> result = std::make_unique<RootSingature>(slotRootParameters, numParameters);
		result->CreateRootSignature(m_pDevice.Get());
		return std::move(result);
	}

	std::unique_ptr<PipelineStateObject> D3DFW::CreatePSO(ID3D12RootSignature* const pRootSignature, const D3D12_INPUT_ELEMENT_DESC* layout, const UINT layoutSize, const UINT renderTargetsNum, DXGI_FORMAT rtvFormats[], DXGI_FORMAT dsvFormat, const UINT SampleMask, const D3D12_PRIMITIVE_TOPOLOGY_TYPE type, ID3DBlob* vsByteCode, ID3DBlob* psByteCode, ID3DBlob* gsByteCode, ID3DBlob* dsByteCode, ID3DBlob* hsByteCode, D3D12_RASTERIZER_DESC rasterizerDesc, D3D12_DEPTH_STENCIL_DESC dsvStateDesc, D3D12_BLEND_DESC blendDesc)
	{
		CONSOLE_MESSAGE("D3DFW is creating Pipeline State");
		std::unique_ptr<PipelineStateObject> result = std::make_unique<PipelineStateObject>(pRootSignature, layout, layoutSize, renderTargetsNum, rtvFormats, dsvFormat);
		result->SetRasterizerState(rasterizerDesc);
		result->SetBlendState(blendDesc);
		result->SetDepthStencilState(dsvStateDesc);
		result->SetSampleDesc(DXGI_SAMPLE_DESC({ m_uSampleCount, m_uQuality }));
		
		if (vsByteCode) result->SetVS(vsByteCode);
		if (psByteCode) result->SetPS(psByteCode);
		if (gsByteCode) result->SetGS(gsByteCode);
		if (dsByteCode) result->SetDS(dsByteCode);
		if (hsByteCode) result->SetHS(hsByteCode);

		result->CreatePSO(m_pDevice.Get());

		return std::move(result);
	}

	std::unique_ptr<ComputePipelineStateObject> D3DFW::CreateComputePSO(ID3D12RootSignature* const pRootSignature, ID3DBlob* csByteCode, const D3D12_PIPELINE_STATE_FLAGS flags, const UINT nodeMask)
	{
		CONSOLE_MESSAGE("D3DFW is creating Compute Pipeline State");
		std::unique_ptr<ComputePipelineStateObject> result = std::make_unique<ComputePipelineStateObject>(pRootSignature, flags, nodeMask);
		
		if (csByteCode) result->SetCS(csByteCode);

		result->CreatePSO(m_pDevice.Get());

		return std::move(result);
	}

	std::unique_ptr<CommandList> D3DFW::CreateList(const D3D12_COMMAND_LIST_TYPE type)
	{
		CONSOLE_MESSAGE("D3DFW is creating Command List");
		return std::make_unique<CommandList>(m_pDevice.Get(), type);
	}

	std::unique_ptr<CommandQueue> D3DFW::CreateQueue(const D3D12_COMMAND_LIST_TYPE type, const D3D12_COMMAND_QUEUE_FLAGS flags, size_t priority, size_t nodeMask)
	{
		CONSOLE_MESSAGE("D3DFW is creating Command Queue");
		return std::make_unique<CommandQueue>(m_pDevice.Get(), type, flags, priority, nodeMask);
	}

	std::unique_ptr<Audio> D3DFW::CreateAudio(const std::wstring& path)
	{
		return std::unique_ptr<Audio>(m_pAudioMananger->CreateAudio(path));
	}

	std::unique_ptr<DepthStencilView> D3DFW::CreateDepthStencilView(const DXGI_FORMAT format, const D3D12_DSV_DIMENSION dimension, const UINT arrSize, const UINT width, const UINT height, const UINT msaaSampleCount, const D3D12_DSV_FLAGS flags)
	{
		CONSOLE_MESSAGE("D3DFW is creating DSV");
		return std::make_unique<DepthStencilView>(m_pDevice.Get(), format, dimension, arrSize, width, height, DXGI_SAMPLE_DESC({ msaaSampleCount, m_uQuality }), flags);
	}

	UINT D3DFW::GetIndexSize(Object* obj, const size_t index) const
	{
		return (UINT)obj->GetObjectParameters(index).IndicesCount;
	}

	UINT D3DFW::GetIndexStartPos(Object* obj, const size_t index) const
	{
		return (UINT)obj->GetObjectParameters(index).IndicesOffset;
	}

	UINT D3DFW::GetVertexStartPos(Object* obj, const size_t index) const
	{
		return (UINT)obj->GetObjectParameters(index).VerticesOffset;
	}

	UINT D3DFW::GetVertexSize(Object* obj, const size_t index) const
	{
		return (UINT)obj->GetObjectParameters(index).VerticesCount;
	}

	UINT D3DFW::GetMaterialIndex(Object* obj, const size_t index) const
	{
		return (UINT)obj->GetObjectParameters(index).MaterialIndex;
	}

	std::unique_ptr<Scene> D3DFW::CreateScene(std::string path, bool neverUpdate, ID3D12GraphicsCommandList* list)
	{
		CONSOLE_MESSAGE("D3DFW is creating Scene");
		return std::make_unique<Scene>(path, m_pDevice.Get(), list, neverUpdate);
	}

	std::unique_ptr<Rectangle> D3DFW::CreateRectangle(bool neverUpdate, ID3D12GraphicsCommandList* list)
	{
		CONSOLE_MESSAGE("D3DFW is creating Rectangle");
		return std::make_unique<Rectangle>(m_pDevice.Get(), list, neverUpdate);
	}

	std::unique_ptr<Cube> D3DFW::CreateCube(bool neverUpdate, ID3D12GraphicsCommandList* list)
	{
		CONSOLE_MESSAGE("D3DFW is creating Cube");
		return std::make_unique<Cube>(m_pDevice.Get(), list, neverUpdate);
	}

	std::unique_ptr<Point> D3DFW::CreatePoint(bool neverUpdate, ID3D12GraphicsCommandList* list)
	{
		return std::make_unique<Point>(m_pDevice.Get(), list, neverUpdate);
	}

	std::unique_ptr<MaterialsManager> D3DFW::CreateMaterialMananger()
	{
		CONSOLE_MESSAGE("D3DFW is creating Materials Mananger");
		return std::make_unique<MaterialsManager>();
	}

	std::unique_ptr<Material> D3DFW::CreateMaterial()
	{
		CONSOLE_MESSAGE("D3DFW is creating Material");
		return std::make_unique<Material>();
	}

	std::shared_ptr<FResource> D3DFW::CreateTexture(std::string path, ID3D12GraphicsCommandList* list)
	{
		return FResource::CreateTextureFromPath(path, m_pDevice.Get(), list);
	}

	std::unique_ptr<FResource> D3DFW::CreateAnonimTexture(const UINT16 arraySize, const DXGI_FORMAT format, const UINT64 width, const UINT64 height, const D3D12_RESOURCE_DIMENSION dimension, const D3D12_RESOURCE_FLAGS resourceFlags, const D3D12_TEXTURE_LAYOUT layout, const D3D12_HEAP_FLAGS heapFlags, const D3D12_HEAP_PROPERTIES* heapProperties, const UINT16 mipLevels)
	{
		CONSOLE_MESSAGE("D3DFW is creating Anonim FResource");
		return std::make_unique<FResource>(m_pDevice.Get(), arraySize, format, width, height, DXGI_SAMPLE_DESC({1, 0}), dimension, resourceFlags, layout, heapFlags, heapProperties, mipLevels);
	}

	std::unique_ptr<FResource> D3DFW::CreateSimpleStructuredBuffer(const UINT64 width)
	{
		return CreateAnonimTexture(1u, DXGI_FORMAT_UNKNOWN,width, 1, D3D12_RESOURCE_DIMENSION_BUFFER, D3D12_RESOURCE_FLAG_NONE, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES);
	}


	void D3DFW::CreateSwapchainData()
	{
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
		rtvHeapDesc.NumDescriptors = BUFFERS_COUNT;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		rtvHeapDesc.NodeMask = 0;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		HRESULT_ASSERT(m_pDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(m_pRTVDescriptorHeap.GetAddressOf())), "RTV descriptor heap create error");

		m_uCurrentBackBufferIndex = 0;

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeap(m_pRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
		for (auto index = 0; index < BUFFERS_COUNT; index++)
		{
			HRESULT_ASSERT(m_pSwapChain->GetBuffer(index, IID_PPV_ARGS(m_aSwapChainRTV[index].GetAddressOf())), "Swapchain get buffer error");
			m_pDevice->CreateRenderTargetView(m_aSwapChainRTV[index].Get(), nullptr, rtvHeap);

			rtvHeap.Offset(1, m_uRTVDescriptorSize);
		}
	}

	void D3DFW::ClearSwapchainData()
	{
		for (auto i = 0; i < BUFFERS_COUNT; ++i)
		{
			m_aSwapChainRTV[i].Reset();
		}
		m_pRTVDescriptorHeap.Reset();
	}

	void D3DFW::SetViewportData(float width, float height)
	{
		m_xMainVP.Height = static_cast<float>(height);
		m_xMainVP.Width = static_cast<float>(width);
		m_xMainVP.MaxDepth = 1.0f;
		m_xMainVP.MinDepth = 0.0f;
		m_xMainVP.TopLeftX = 0.0f;
		m_xMainVP.TopLeftY = 0.0f;
	}

	void D3DFW::SetRectData(float width, float height)
	{
		m_xMainRect.left = 0;
		m_xMainRect.right = width;
		m_xMainRect.top = 0;
		m_xMainRect.bottom = height;
	}


}