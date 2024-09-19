#include "pch.h"
#include "D3DFW.h"


namespace FD3DW
{
	D3D12_CPU_DESCRIPTOR_HANDLE D3DFW::GetCurrBackBufferView() const noexcept
	{
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(pRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), currentBackBufferIndex, rtvDescriptorSize);
	}

	ID3D12Resource* D3DFW::GetCurrBackBufferResource() const noexcept
	{
		return pSwapChainRTV[currentBackBufferIndex].Get();
	}


	DXGI_FORMAT D3DFW::GetMainRTVFormat() const noexcept
	{
		return DXGI_FORMAT_R8G8B8A8_UNORM;
	}

	AudioMananger* D3DFW::GetAudioMananger() const noexcept
	{
		return pAudioMananger.get();
	}

	NVSDK_NGX_Parameter* D3DFW::GetNGXParameter() const noexcept
	{
		return pNGXParams;
	}

	NVSDK_NGX_Handle* D3DFW::GetNGXHandle() const noexcept
	{
		return pNGXHandle;
	}

	NVSDK_NGX_PerfQuality_Value D3DFW::GetDLSSQualityValue() const noexcept
	{
		return value;
	}

	void D3DFW::PresentSwapchain()
	{
		HRESULT_ASSERT(pSwapChain->Present(UINT(vSync), 0), "Swapchain present error");
		currentBackBufferIndex = (currentBackBufferIndex + 1) % BUFFERS_COUNT;
	}

	void D3DFW::BeginDraw(ID3D12GraphicsCommandList* pCommandList)
	{
		pCommandList->ResourceBarrier(1, &keep(CD3DX12_RESOURCE_BARRIER::Transition(pSwapChainRTV[currentBackBufferIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET)));
	}

	void D3DFW::EndDraw(ID3D12GraphicsCommandList* pCommandList)
	{
		pCommandList->ResourceBarrier(1, &keep(CD3DX12_RESOURCE_BARRIER::Transition(pSwapChainRTV[currentBackBufferIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT)));
	}

	void D3DFW::BindMainViewPort(ID3D12GraphicsCommandList* pCommandList)
	{
		pCommandList->RSSetViewports(1, &mainVP);
	}

	void D3DFW::BindMainRect(ID3D12GraphicsCommandList* pCommandList)
	{
		pCommandList->RSSetScissorRects(1, &mainRect);
	}

	void D3DFW::BindListToMainQueue(CommandList* pCommandList)
	{
		pCommandQueue->BindCommandList(pCommandList);
	}

	void D3DFW::UnbindListFromMainQueue(CommandList* pCommandList)
	{
		pCommandQueue->UnbindCommandList(pCommandList);
	}

	void D3DFW::ExecuteMainQueue()
	{
		pCommandQueue->ExecuteQueue(true);
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
		vSync = enable;
	}

	// FUTURE
	void D3DFW::SetDLSS(UINT SamplesCount)
	{
	}

	UINT D3DFW::GetMSAAQualitySupport(const UINT msaaSamples) const
	{
		D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS qualityLevels;
		qualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		qualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
		qualityLevels.NumQualityLevels = 0;
		qualityLevels.SampleCount = msaaSamples;

		HRESULT_ASSERT(pDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &qualityLevels, sizeof(qualityLevels)), "MSAA check quality error");

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
		return true;
	}

	void D3DFW::ChildRelease()
	{
		UserClose();
		CONSOLE_MESSAGE("USER CLOSED");

		CONSOLE_MESSAGE("FRAMEWORK CLOSED");
	}

	void D3DFW::ChildKeyPressed(WPARAM param)
	{
		UserKeyPressed(param);
	}

	void D3DFW::ChildSIZE()
	{
		const auto& WndSet = WNDSettings();
		mainProjectionMatrix = dx::XMMatrixPerspectiveFovLH(M_PI_2_F, (float)WndSet.width / WndSet.height, 1.0f, 10000.0f);
	}

	void D3DFW::ChildENTERSIZE()
	{
	}

	void D3DFW::ChildEXITSIZE()
	{
		const auto& WndSet = WNDSettings();

		pSwapChain->ResizeBuffers(BUFFERS_COUNT, WndSet.width, WndSet.height, GetMainRTVFormat(), DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
		mainVP.Height = static_cast<float>(WndSet.height);
		mainVP.Width = static_cast<float>(WndSet.width);

		mainRect.left = 0;
		mainRect.right = WndSet.width;
		mainRect.top = 0;
		mainRect.bottom = WndSet.height;

		mainProjectionMatrix = dx::XMMatrixPerspectiveFovLH(M_PI_2_F, (float)WndSet.width / WndSet.height, 1.0f, 10000.0f);

		UserResizeUpdate();
	}

	void D3DFW::ChildMOUSEUP(WPARAM btnState, int x, int y)
	{
		UserMouseUp(btnState, x, y);
	}

	void D3DFW::ChildMOUSEDOWN(WPARAM btnState, int x, int y)
	{
		UserMouseDown(btnState, x, y);
	}

	void D3DFW::ChildMOUSEMOVE(WPARAM btnState, int x, int y)
	{
		UserMouseMoved(btnState, x, y);
	}

	bool D3DFW::InitAudioMananger()
	{
		pAudioMananger = std::make_unique<AudioMananger>();
		return pAudioMananger ? true : false;
	}

	bool D3DFW::InitD3D()
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
		rtvDescriptorSize = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		cbvsrvuavDescriptorSize = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		dsvDescriptorSize = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

		pCommandQueue = CreateQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);

		const auto& WndSet = WNDSettings();

		DXGI_SWAP_CHAIN_DESC swapChainDesc;
		swapChainDesc.BufferDesc.Width = WndSet.width;
		swapChainDesc.BufferDesc.Height = WndSet.height;
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
		swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		swapChainDesc.SampleDesc.Count = SampleCount = 1;           //MSAA DISABLED
		swapChainDesc.SampleDesc.Quality = Quality = 0;         //MSAA DISABLED
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		SETHWND(swapChainDesc.OutputWindow);
		swapChainDesc.BufferCount = BUFFERS_COUNT;
		swapChainDesc.Windowed = !WndSet.fullScreen;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

		HRESULT_ASSERT(pFactory->CreateSwapChain(pCommandQueue->GetQueue(), &swapChainDesc, pSwapChain.GetAddressOf()), "Swapchain create error");

		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
		rtvHeapDesc.NumDescriptors = BUFFERS_COUNT;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		rtvHeapDesc.NodeMask = 0;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		HRESULT_ASSERT(pDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(pRTVDescriptorHeap.GetAddressOf())), "RTV descriptor heap create error");

		currentBackBufferIndex = 0;


		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeap(pRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

		for (UINT index = 0; index < BUFFERS_COUNT; index++)
		{
			HRESULT_ASSERT(pSwapChain->GetBuffer(index, IID_PPV_ARGS(pSwapChainRTV[index].GetAddressOf())), "Swapchain buffer get error");
			pDevice->CreateRenderTargetView(pSwapChainRTV[index].Get(), nullptr, rtvHeap);

			rtvHeap.Offset(1, rtvDescriptorSize);
		}

		mainVP.Height = static_cast<float>(WndSet.height);
		mainVP.Width = static_cast<float>(WndSet.width);
		mainVP.MaxDepth = 1.0f;
		mainVP.MinDepth = 0.0f;
		mainVP.TopLeftX = 0.0f;
		mainVP.TopLeftY = 0.0f;

		mainRect.left = 0;
		mainRect.right = WndSet.width;
		mainRect.top = 0;
		mainRect.bottom = WndSet.height;

		return true;
	}

	
	void D3DFW::Update()
	{
		/////////////////// 
		// USER DRAW
		UserLoop();
		//////////////////

		ExecuteMainQueue();

		PresentSwapchain();
	}


	

	void D3DFW::OnResizeDLSS(CommandList* pCommandList)
	{
		if (value)
		{
			/*const auto& WndSet = WNDSettings();
			UINT renderWMax, renderHMax, renderWMin, renderHMin;
			float sharpness = 0.0F;

			NVSDK_NGX_Result result = NGX_DLSS_GET_OPTIMAL_SETTINGS(pNGXParams, WndSet.width, WndSet.height, value, &WndSet.dlssWidth, &WndSet.dlssHeight, &renderWMax, &renderHMax, &renderWMin, &renderHMin, &sharpness);
			if (result != NVSDK_NGX_Result_Success) {
				return;
			}

			NVSDK_NGX_DLSS_Create_Params featureDesc = {};
			featureDesc.Feature.InWidth = WndSet.dlssWidth;
			featureDesc.Feature.InHeight = WndSet.dlssHeight;
			featureDesc.Feature.InTargetWidth = WndSet.width;
			featureDesc.Feature.InTargetHeight = WndSet.height;
			featureDesc.Feature.InPerfQualityValue = value;
			featureDesc.InFeatureCreateFlags = NVSDK_NGX_DLSS_Feature_Flags_AutoExposure
				| NVSDK_NGX_DLSS_Feature_Flags_MVLowRes
				| NVSDK_NGX_DLSS_Feature_Flags_IsHDR;

			if(!pCommandList->TryCloseList())
				pCommandList->ResetList();
			
			result = NGX_D3D12_CREATE_DLSS_EXT(pCommandList->GetPtrCommandList(), 1, 1, &pNGXHandle, pNGXParams, &featureDesc);
			if (result != NVSDK_NGX_Result_Success) {
				return;
			}

			pCommandList->ExecuteList(pCommandQueue->GetQueue());
			pCommandQueue->FlushQueue();*/
		}
	}

	bool D3DFW::InitNGX(CommandList* pCommandList)
	{
		int updateDriver = 0;
		UINT minDriverMajorI = 0;
		UINT minDriverMinorI = 0;
		UINT dlssSupported = 0;

		NVSDK_NGX_Result result = NVSDK_NGX_D3D12_Init_with_ProjectID("a0f57b54-1daf-4934-90ae-c4035c19df04", NVSDK_NGX_ENGINE_TYPE_CUSTOM, "DebugVersion", L"", pDevice.Get());
		if (result != NVSDK_NGX_Result_Success)
			return false;

		result = NVSDK_NGX_D3D12_GetCapabilityParameters(&pNGXParams);
		if (result != NVSDK_NGX_Result_Success)
			return false;

		result = pNGXParams->Get(NVSDK_NGX_Parameter_SuperSampling_NeedsUpdatedDriver, &updateDriver);
		NVSDK_NGX_Result minDriverMajor = pNGXParams->Get(NVSDK_NGX_Parameter_SuperSampling_MinDriverVersionMajor, &minDriverMajorI);
		NVSDK_NGX_Result minDriverMinor = pNGXParams->Get(NVSDK_NGX_Parameter_SuperSampling_MinDriverVersionMinor, &minDriverMinorI);
		if (NVSDK_NGX_SUCCEED(result) && updateDriver && NVSDK_NGX_SUCCEED(minDriverMajor) && NVSDK_NGX_SUCCEED(minDriverMinor))
		{
			return false;
		}

		result = pNGXParams->Get(NVSDK_NGX_Parameter_SuperSampling_Available, &dlssSupported);
		if (NVSDK_NGX_FAILED(result) || !dlssSupported)
			return false;
		
		result = pNGXParams->Get(NVSDK_NGX_Parameter_SuperSampling_FeatureInitResult, &dlssSupported);
		if (NVSDK_NGX_FAILED(result) || !dlssSupported)
			return false;

		value = NVSDK_NGX_PerfQuality_Value(dlssSupported);
		
		OnResizeDLSS(pCommandList);

		return true;
	}

	const UINT D3DFW::Get_CBV_SRV_UAV_DescriptorSize() const noexcept
	{
		return cbvsrvuavDescriptorSize;
	}

	const UINT D3DFW::Get_RTV_DescriptorSize() const noexcept
	{
		return rtvDescriptorSize;
	}

	const UINT D3DFW::Get_DSV_DescriptorSize() const noexcept
	{
		return dsvDescriptorSize;
	}

	ID3D12Device* D3DFW::GetDevice() const noexcept
	{
		return pDevice.Get();
	}

	WindowSettings D3DFW::GetMainWNDSettings() const noexcept
	{
		return WNDSettings();
	}

	dx::XMMATRIX D3DFW::GetMainProjectionMatrix() const noexcept
	{
		return mainProjectionMatrix;
	}

	D3D12_VIEWPORT D3DFW::GetMainViewPort() const noexcept
	{
		return mainVP;
	}

	D3D12_RECT D3DFW::GetMainRect() const noexcept
	{
		return mainRect;
	}

	std::unique_ptr<RenderTarget> D3DFW::CreateRenderTarget(const DXGI_FORMAT format, const D3D12_RTV_DIMENSION dimension, const UINT arrSize, const UINT width, const UINT height, const UINT msaaSampleCount)
	{
		CONSOLE_MESSAGE("D3DFW is creating RTV");
		return std::make_unique<RenderTarget>(pDevice.Get(), format, dimension, arrSize, width, height, DXGI_SAMPLE_DESC({ msaaSampleCount, Quality }));
	}

	std::unique_ptr<RTVPacker> D3DFW::CreateRTVPack(const UINT descriptorsCount, const UINT NodeMask)
	{
		CONSOLE_MESSAGE("D3DFW is creating RTV pack");
		return std::make_unique<RTVPacker>(Get_RTV_DescriptorSize(), descriptorsCount, NodeMask, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, pDevice.Get());
	}

	std::unique_ptr<DSVPacker> D3DFW::CreateDSVPack(const UINT descriptorsCount, const UINT NodeMask)
	{
		CONSOLE_MESSAGE("D3DFW is creating DSV pack");
		return std::make_unique<DSVPacker>(Get_RTV_DescriptorSize(), descriptorsCount, NodeMask, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, pDevice.Get());
	}

	std::unique_ptr<SRVPacker> D3DFW::CreateSRVPack(const UINT descriptorsCount, const UINT NodeMask)
	{
		CONSOLE_MESSAGE("D3DFW is creating SRV pack");
		return std::make_unique<SRVPacker>(Get_CBV_SRV_UAV_DescriptorSize(), descriptorsCount, NodeMask, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, pDevice.Get());
	}

	std::unique_ptr<CBVPacker> D3DFW::CreateCBVPack(const UINT descriptorsCount, const UINT NodeMask)
	{
		CONSOLE_MESSAGE("D3DFW is creating CBV pack");
		return std::make_unique<CBVPacker>(Get_CBV_SRV_UAV_DescriptorSize(), descriptorsCount, NodeMask, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, pDevice.Get());
	}

	std::unique_ptr<UAVPacker> D3DFW::CreateUAVPack(const UINT descriptorsCount, const UINT NodeMask)
	{
		CONSOLE_MESSAGE("D3DFW is creating UAV pack");
		return std::make_unique<UAVPacker>(Get_CBV_SRV_UAV_DescriptorSize(), descriptorsCount, NodeMask, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, pDevice.Get());
	}

	std::unique_ptr<SamplerPacker> D3DFW::CreateSamplerPack(const UINT descriptorsCount, const UINT NodeMask)
	{
		CONSOLE_MESSAGE("D3DFW is creating Samplers pack");
		return std::make_unique<SamplerPacker>(Get_CBV_SRV_UAV_DescriptorSize(), descriptorsCount, NodeMask, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, pDevice.Get());
	}

	std::unique_ptr<RootSingature> D3DFW::CreateRootSignature(CD3DX12_ROOT_PARAMETER* slotRootParameters, const UINT numParameters)
	{
		CONSOLE_MESSAGE("D3DFW is creating Root Signature");
		std::unique_ptr<RootSingature> result = std::make_unique<RootSingature>(slotRootParameters, numParameters);
		result->CreateRootSignature(pDevice.Get());
		return std::move(result);
	}

	std::unique_ptr<PipelineStateObject> D3DFW::CreatePSO(ID3D12RootSignature* const pRootSignature, const D3D12_INPUT_ELEMENT_DESC* layout, const UINT layoutSize, const UINT renderTargetsNum, DXGI_FORMAT rtvFormats[], DXGI_FORMAT dsvFormat, const UINT SampleMask, const D3D12_PRIMITIVE_TOPOLOGY_TYPE type, ID3DBlob* vsByteCode, ID3DBlob* psByteCode, ID3DBlob* gsByteCode, ID3DBlob* dsByteCode, ID3DBlob* hsByteCode, D3D12_RASTERIZER_DESC rasterizerDesc, D3D12_DEPTH_STENCIL_DESC dsvStateDesc, D3D12_BLEND_DESC blendDesc)
	{
		CONSOLE_MESSAGE("D3DFW is creating Pipeline State");
		std::unique_ptr<PipelineStateObject> result = std::make_unique<PipelineStateObject>(pRootSignature, layout, layoutSize, renderTargetsNum, rtvFormats, dsvFormat);
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

	std::unique_ptr<ComputePipelineStateObject> D3DFW::CreateComputePSO(ID3D12RootSignature* const pRootSignature, ID3DBlob* csByteCode, const D3D12_PIPELINE_STATE_FLAGS flags, const UINT nodeMask)
	{
		CONSOLE_MESSAGE("D3DFW is creating Compute Pipeline State");
		std::unique_ptr<ComputePipelineStateObject> result = std::make_unique<ComputePipelineStateObject>(pRootSignature, flags, nodeMask);
		
		if (csByteCode) result->SetCS(csByteCode);

		result->CreatePSO(pDevice.Get());

		return std::move(result);
	}

	std::unique_ptr<CommandList> D3DFW::CreateList(const D3D12_COMMAND_LIST_TYPE type)
	{
		CONSOLE_MESSAGE("D3DFW is creating Command List");
		return std::make_unique<CommandList>(pDevice.Get(), type);
	}

	std::unique_ptr<CommandQueue> D3DFW::CreateQueue(const D3D12_COMMAND_LIST_TYPE type, const D3D12_COMMAND_QUEUE_FLAGS flags, size_t priority, size_t nodeMask)
	{
		CONSOLE_MESSAGE("D3DFW is creating Command Queue");
		return std::make_unique<CommandQueue>(pDevice.Get(), type, flags, priority, nodeMask);
	}

	std::unique_ptr<Audio> D3DFW::CreateAudio(const std::wstring& path)
	{
		return std::unique_ptr<Audio>(pAudioMananger->CreateAudio(path));
	}

	std::unique_ptr<DepthStencilView> D3DFW::CreateDepthStencilView(const DXGI_FORMAT format, const D3D12_DSV_DIMENSION dimension, const UINT arrSize, const UINT width, const UINT height, const UINT msaaSampleCount, const D3D12_DSV_FLAGS flags)
	{
		CONSOLE_MESSAGE("D3DFW is creating DSV");
		return std::make_unique<DepthStencilView>(pDevice.Get(), format, dimension, arrSize, width, height, DXGI_SAMPLE_DESC({ msaaSampleCount, Quality }), flags);
	}

	UINT D3DFW::GetIndexSize(Object* obj, const size_t index) const
	{
		return (UINT)obj->GetObjectParameters(index).indicesCount;
	}

	UINT D3DFW::GetIndexStartPos(Object* obj, const size_t index) const
	{
		return (UINT)obj->GetObjectParameters(index).indicesOffset;
	}

	UINT D3DFW::GetVertexStartPos(Object* obj, const size_t index) const
	{
		return (UINT)obj->GetObjectParameters(index).verticesOffset;
	}

	UINT D3DFW::GetVertexSize(Object* obj, const size_t index) const
	{
		return (UINT)obj->GetObjectParameters(index).verticesCount;
	}

	UINT D3DFW::GetMaterialIndex(Object* obj, const size_t index) const
	{
		return (UINT)obj->GetObjectParameters(index).materialIndex;
	}

	std::unique_ptr<Scene> D3DFW::CreateScene(std::string path, bool neverUpdate, ID3D12GraphicsCommandList* list)
	{
		CONSOLE_MESSAGE("D3DFW is creating Scene");
		return std::make_unique<Scene>(path, pDevice.Get(), list, neverUpdate);
	}

	std::unique_ptr<Rectangle> D3DFW::CreateRectangle(bool neverUpdate, ID3D12GraphicsCommandList* list)
	{
		CONSOLE_MESSAGE("D3DFW is creating Rectangle");
		return std::make_unique<Rectangle>(pDevice.Get(), list, neverUpdate);
	}

	std::unique_ptr<Cube> D3DFW::CreateCube(bool neverUpdate, ID3D12GraphicsCommandList* list)
	{
		CONSOLE_MESSAGE("D3DFW is creating Cube");
		return std::make_unique<Cube>(pDevice.Get(), list, neverUpdate);
	}

	std::unique_ptr<Point> D3DFW::CreatePoint(bool neverUpdate, ID3D12GraphicsCommandList* list)
	{
		return std::make_unique<Point>(pDevice.Get(), list, neverUpdate);
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
		return FResource::CreateTextureFromPath(path, pDevice.Get(), list);
	}

	std::unique_ptr<FResource> D3DFW::CreateAnonimTexture(const UINT16 arraySize, const DXGI_FORMAT format, const UINT64 width, const UINT64 height, const D3D12_RESOURCE_DIMENSION dimension, const D3D12_RESOURCE_FLAGS resourceFlags, const D3D12_TEXTURE_LAYOUT layout, const D3D12_HEAP_FLAGS heapFlags, const D3D12_HEAP_PROPERTIES* heapProperties, const UINT16 mipLevels)
	{
		CONSOLE_MESSAGE("D3DFW is creating Anonim FResource");
		return std::make_unique<FResource>(pDevice.Get(), arraySize, format, width, height, DXGI_SAMPLE_DESC({1, 0}), dimension, resourceFlags, layout, heapFlags, heapProperties, mipLevels);
	}

	std::unique_ptr<FResource> D3DFW::CreateSimpleStructuredBuffer(const UINT64 width)
	{
		return CreateAnonimTexture(1u, DXGI_FORMAT_UNKNOWN,width, 1, D3D12_RESOURCE_DIMENSION_BUFFER, D3D12_RESOURCE_FLAG_NONE, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES);
	}

}