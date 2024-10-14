#pragma once
#include "pch.h"

//////////////////////////////
//			WINWINDOW
#include "WinWindow/WinWindow.h"
//////////////////////////////

#include "Utilites/Macroses.h"
#include "Utilites/Structures.h"

#include "Objects/SimpleObjects.h"
#include "Objects/Scene.h"

#include "GraphicUtilites/BufferMananger.h"
#include "GraphicUtilites/Shader.h"
#include "GraphicUtilites/CommandQueue.h"
#include "GraphicUtilites/FResource.h"
#include "GraphicUtilites/ResourcePacker.h"
#include "GraphicUtilites/PipelineStateObject.h"
#include "GraphicUtilites/RootSignature.h"
#include "GraphicUtilites/RenderTarget.h"
#include "GraphicUtilites/DepthStencilView.h"
#include "GraphicUtilites/InputLayout.h"

#include "Utilites/AudioMananger.h"

namespace FD3DW
{


	class D3DFW : virtual public FDWWIN::WinWindow
	{
	public:
		D3DFW()=default;
		virtual ~D3DFW()=default;


		virtual void UserInit() = 0;
		virtual void UserLoop() = 0;
		virtual void UserClose() = 0;
		virtual void UserMouseDown(WPARAM btnState, int x, int y) = 0;
		virtual void UserMouseUp(WPARAM btnState, int x, int y) = 0;
		virtual void UserMouseMoved(WPARAM btnState, int x, int y) = 0;
		virtual void UserKeyPressed(WPARAM wParam) = 0;
		virtual void UserResizeUpdate() = 0;

		UINT GetMSAAQualitySupport(const UINT msaaSamples) const;


	protected:
		
		///////////////////
		//		IN PROGRESS (NOT WORKING NOW!!!!)
		bool InitNGX(CommandList* pCommandList);   // for DLSS
		
		// use this on resizing window IN PROGRESS (NOT WORKING NOW!!!!)
		void OnResizeDLSS(CommandList* pCommandList); // for DLSS


	private:  //METHODS

		////////////////////////////
		///////		FROM WIN WINDOW
		virtual void ChildLoop() override;
		virtual bool ChildInit() override;
		virtual void ChildRelease() override;
		virtual void ChildKeyPressed(WPARAM) override;
		virtual void ChildSIZE() override;
		virtual void ChildENTERSIZE() override;
		virtual void ChildEXITSIZE() override;
		virtual void ChildMOUSEUP(WPARAM btnState, int x, int y) override;
		virtual void ChildMOUSEDOWN(WPARAM btnState, int x, int y) override;
		virtual void ChildMOUSEMOVE(WPARAM btnState, int x, int y) override;

		bool InitAudioMananger();
		bool InitD3D();
		void Update();

	private:

		std::unique_ptr<AudioMananger> pAudioMananger;

		// WINDOW
		dx::XMMATRIX mainProjectionMatrix;
		D3D12_VIEWPORT mainVP;
		D3D12_RECT mainRect;

		//D3D12

		wrl::ComPtr<ID3D12Device> pDevice;
		wrl::ComPtr<IDXGIFactory4> pFactory;
		wrl::ComPtr<IDXGISwapChain> pSwapChain;
		wrl::ComPtr<ID3D12DescriptorHeap> pRTVDescriptorHeap;
		wrl::ComPtr<ID3D12Resource> pSwapChainRTV[BUFFERS_COUNT];

		std::unique_ptr<CommandQueue> pCommandQueue;

		bool vSync = false;

		//NGX

		NVSDK_NGX_Parameter* pNGXParams;
		NVSDK_NGX_Handle* pNGXHandle;
		NVSDK_NGX_PerfQuality_Value value = NVSDK_NGX_PerfQuality_Value_MaxPerf;
		//////////////////////////

		
		//////////////////////////

		UINT currentBackBufferIndex;

		//////////////////////////
		

		UINT rtvDescriptorSize;
		UINT dsvDescriptorSize;
		UINT cbvsrvuavDescriptorSize;

		UINT SampleCount;
		UINT Quality;

	protected:

		const UINT					Get_CBV_SRV_UAV_DescriptorSize()	const noexcept;
		const UINT					Get_RTV_DescriptorSize()			const noexcept;
		const UINT					Get_DSV_DescriptorSize()			const noexcept;
		ID3D12Device*				GetDevice()							const noexcept;
		WindowSettings				GetMainWNDSettings()				const noexcept;
		dx::XMMATRIX				GetMainProjectionMatrix()			const noexcept;
		D3D12_VIEWPORT				GetMainViewPort()					const noexcept;
		D3D12_RECT					GetMainRect()						const noexcept;
		D3D12_CPU_DESCRIPTOR_HANDLE GetCurrBackBufferView()				const noexcept;
		ID3D12Resource*			    GetCurrBackBufferResource()			const noexcept;
		DXGI_FORMAT					GetMainRTVFormat()					const noexcept;
		AudioMananger*				GetAudioMananger()					const noexcept;

		//////////////////
		//		DLSS
		NVSDK_NGX_Parameter*		GetNGXParameter()					const noexcept;
		NVSDK_NGX_Handle*			GetNGXHandle()						const noexcept;
		NVSDK_NGX_PerfQuality_Value	GetDLSSQualityValue()				const noexcept;
		//////////////////

		void				PresentSwapchain();
		void				BeginDraw(ID3D12GraphicsCommandList* pCommandList);
		void				EndDraw(ID3D12GraphicsCommandList* pCommandList);
		void				BindMainViewPort(ID3D12GraphicsCommandList* pCommandList);
		void				BindMainRect(ID3D12GraphicsCommandList* pCommandList);
		void				BindListToMainQueue(CommandList* pCommandList);
		void				UnbindListFromMainQueue(CommandList* pCommandList);
		void				ExecuteMainQueue();
		void				SetVSync(bool enable);
		//IN PROGRESS (NOT WORKING NOW!!!!)
		void				SetDLSS(UINT SamplesCount);
		

		////////////////////////////////////
		// FACADE
		UINT GetIndexSize(Object* obj, const size_t index)		const;
		UINT GetIndexStartPos(Object* obj, const size_t index)	const;
		UINT GetVertexStartPos(Object* obj, const size_t index)	const;
		UINT GetVertexSize(Object* obj, const size_t index)		const;
		UINT GetMaterialIndex(Object* obj, const size_t index)    const;

		std::unique_ptr<Scene>				CreateScene(std::string path, bool neverUpdate, ID3D12GraphicsCommandList* list);
		std::unique_ptr<Rectangle>			CreateRectangle(bool neverUpdate, ID3D12GraphicsCommandList* list);
		std::unique_ptr<Cube>				CreateCube(bool neverUpdate, ID3D12GraphicsCommandList* list);
		std::unique_ptr<Point>				CreatePoint(bool neverUpdate, ID3D12GraphicsCommandList* list);
		std::unique_ptr<MaterialsManager>	CreateMaterialMananger();
		std::unique_ptr<Material>			CreateMaterial();
		std::shared_ptr<FResource>			CreateTexture(std::string path, ID3D12GraphicsCommandList* list);
		std::unique_ptr<FResource>			CreateAnonimTexture(const UINT16 arraySize, const DXGI_FORMAT format,
																const UINT64 width, const UINT64 height,
																const D3D12_RESOURCE_DIMENSION dimension,
																const D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE,
																const D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
																const D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_NONE,
																const D3D12_HEAP_PROPERTIES* heapProperties = &keep(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)),
																const UINT16 mipLevels = 1);
		std::unique_ptr<FResource>			CreateSimpleStructuredBuffer(const UINT64 width);

		std::unique_ptr<RenderTarget>		CreateRenderTarget(const DXGI_FORMAT format, const D3D12_RTV_DIMENSION dimension, const UINT arrSize,
																const UINT width, const UINT height, const UINT msaaSampleCount = 1u);
		std::unique_ptr<DepthStencilView>	CreateDepthStencilView(const DXGI_FORMAT format, const D3D12_DSV_DIMENSION dimension, const UINT arrSize,
																const UINT width, const UINT height, const UINT msaaSampleCount = 1u, const D3D12_DSV_FLAGS flags = D3D12_DSV_FLAG_NONE);

		std::unique_ptr<RTVPacker> CreateRTVPack(const UINT descriptorsCount, const UINT NodeMask = 0);
		std::unique_ptr<DSVPacker> CreateDSVPack(const UINT descriptorsCount, const UINT NodeMask = 0);
		std::unique_ptr<SRVPacker> CreateSRVPack(const UINT descriptorsCount, const UINT NodeMask = 0);
		std::unique_ptr<CBVPacker> CreateCBVPack(const UINT descriptorsCount, const UINT NodeMask = 0);
		std::unique_ptr<UAVPacker> CreateUAVPack(const UINT descriptorsCount, const UINT NodeMask = 0);
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
			D3D12_DEPTH_STENCIL_DESC dsvStateDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT),
			D3D12_BLEND_DESC blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT)
			);

		std::unique_ptr<ComputePipelineStateObject> CreateComputePSO(ID3D12RootSignature* const pRootSignature,
			ID3DBlob* csByteCode = nullptr, const D3D12_PIPELINE_STATE_FLAGS flags = D3D12_PIPELINE_STATE_FLAG_NONE, const UINT nodeMask = 0);

		std::unique_ptr<CommandList> CreateList(const D3D12_COMMAND_LIST_TYPE type);
		std::unique_ptr<CommandQueue> CreateQueue(const D3D12_COMMAND_LIST_TYPE type, const D3D12_COMMAND_QUEUE_FLAGS flags = D3D12_COMMAND_QUEUE_FLAG_NONE, size_t priority = 0, size_t nodeMask = 0);

		std::unique_ptr<Audio>		CreateAudio(const std::wstring& path);

		//
		////////////////////////////////////
		

		////////////////////////////////////
		//              TEMPLATES
		template<typename T, typename ...Args>
		inline std::unique_ptr<T> MakePointer(Args && ...arguments)
		{
			return std::make_unique<T>((arguments)...);
		}

		template<typename T>
		inline std::unique_ptr<UploadBuffer<T>> CreateConstantBuffer(const size_t&& elementCount)
		{
			CONSOLE_MESSAGE("D3DFW is creating Constant Buffer");
			return std::make_unique<UploadBuffer<T>>(pDevice.Get(), std::forward<decltype(elementCount)>(elementCount), true);
		}


	};

	





}