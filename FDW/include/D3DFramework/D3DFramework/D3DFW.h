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

#include "GraphicUtilites/BufferManager.h"
#include "GraphicUtilites/CommandQueue.h"
#include "GraphicUtilites/FResource.h"
#include "GraphicUtilites/ResourcePacker.h"
#include "GraphicUtilites/RenderTarget.h"
#include "GraphicUtilites/DepthStencilView.h"
#include "GraphicUtilites/InputLayout.h"
#include "GraphicUtilites/PipelineObject.h"

#include "Utilites/AudioManager.h"

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

		UINT GetMSAAQualitySupport(const UINT msaaSamples) const;

	protected:  //METHODS

		////////////////////////////
		///////		FROM WIN WINDOW
		virtual void ChildLoop() override;
		virtual bool ChildInit() override;
		virtual void ChildRelease() override;

		bool InitAudioMananger();
		bool InitD3D();
		void Update();

	private:
		void ResizeHandler();
		
	private:
		std::unique_ptr<AudioManager> m_pAudioMananger;

		// WINDOW
		D3D12_VIEWPORT m_xMainVP;
		D3D12_RECT m_xMainRect;

		//D3D12

		wrl::ComPtr<ID3D12Device> m_pDevice;
		wrl::ComPtr<IDXGIFactory4> m_pFactory;
		wrl::ComPtr<IDXGISwapChain> m_pSwapChain;
		wrl::ComPtr<ID3D12DescriptorHeap> m_pRTVDescriptorHeap;
		wrl::ComPtr<ID3D12Resource> m_aSwapChainRTV[BUFFERS_COUNT];

		std::unique_ptr<CommandQueue> m_pCommandQueue;

		bool m_bVSync = false;

		//////////////////////////

		UINT m_uCurrentBackBufferIndex = 0;
		
		//////////////////////////
		

		UINT m_uRTVDescriptorSize;
		UINT m_uDSVDescriptorSize;
		UINT m_uCBV_SRV_UAVDescriptorSize;

		UINT m_uSampleCount;
		UINT m_uQuality;

	private:
		CommandList* m_pBindedMainCommandList = nullptr;

	private:
		void				PresentSwapchain();

	protected:
		void				BeginDraw(ID3D12GraphicsCommandList* pCommandList);
		void				EndDraw(ID3D12GraphicsCommandList* pCommandList);

	public:

		const UINT					Get_CBV_SRV_UAV_DescriptorSize()	const noexcept;
		const UINT					Get_RTV_DescriptorSize()			const noexcept;
		const UINT					Get_DSV_DescriptorSize()			const noexcept;
		ID3D12Device*				GetDevice()							const noexcept;
		wrl::ComPtr<ID3D12Device>	GetComPtrDevice()					const noexcept;
		IDXGISwapChain*				GetSwapChain()						const noexcept;
		wrl::ComPtr<IDXGISwapChain> GetComPtrSwapChain()				const noexcept;
		UINT						GetCurrentBackBufferIndex()			const noexcept;
		WindowSettings				GetMainWNDSettings()				const noexcept;
		D3D12_VIEWPORT				GetMainViewPort()					const noexcept;
		D3D12_RECT					GetMainRect()						const noexcept;
		D3D12_CPU_DESCRIPTOR_HANDLE GetCurrBackBufferView()				const noexcept;
		ID3D12Resource*			    GetCurrBackBufferResource()			const noexcept;
		DXGI_FORMAT					GetMainRTVFormat()					const noexcept;
		AudioManager*				GetAudioMananger()					const noexcept;
		ID3D12CommandQueue*			GetCommandQueue()					const noexcept;
		ID3D12GraphicsCommandList*	GetBindedCommandList()				const noexcept;

	protected:
		void				BindMainViewPort(ID3D12GraphicsCommandList* pCommandList);
		void				BindMainRect(ID3D12GraphicsCommandList* pCommandList);
		void				BindListToMainQueue(CommandList* pCommandList);
		void				BindMainCommandList(CommandList* pCommandList);
		void				UnbindListFromMainQueue(CommandList* pCommandList);
		void				ExecuteMainQueue();
		void				SetVSync(bool enable);
		
	public:
		////////////////////////////////////
		// FACADE
		UINT GetIndexSize(Object* obj, const size_t index)		const;
		UINT GetIndexStartPos(Object* obj, const size_t index)	const;
		UINT GetVertexStartPos(Object* obj, const size_t index)	const;
		UINT GetVertexSize(Object* obj, const size_t index)		const;
		UINT GetMaterialIndex(Object* obj, const size_t index)    const;

	public:
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

		std::unique_ptr<CommandList> CreateList(const D3D12_COMMAND_LIST_TYPE type);
		std::unique_ptr<CommandQueue> CreateQueue(const D3D12_COMMAND_LIST_TYPE type, const D3D12_COMMAND_QUEUE_FLAGS flags = D3D12_COMMAND_QUEUE_FLAG_NONE, size_t priority = 0, size_t nodeMask = 0);

		std::unique_ptr<Audio>		CreateAudio(const std::wstring& path);

		//
		////////////////////////////////////
		
		protected:

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
			return std::make_unique<UploadBuffer<T>>(m_pDevice.Get(), std::forward<decltype(elementCount)>(elementCount), true);
		}

		///////////////////////////////////


		private:

			void CreateSwapchainData();
			void ClearSwapchainData();
			void SetViewportData(float width, float height);
			void SetRectData(float width, float height);
			
		// MESSAGE LAYER
		private:

			class D3DFWMessageLayer : virtual public FDWWIN::WinWindowInputLayer {				
			public:
				D3DFWMessageLayer(std::function<void(void)> exitSize);
				virtual ~D3DFWMessageLayer() = default;

			public:
				virtual bool ProcessInput(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;

			private:
				std::function<void(void)> m_hExitSizeHandler = nullptr;
			};

			std::unique_ptr<D3DFWMessageLayer> m_pMessageLayer = nullptr;

			void InitMessageLayer();
			void DestroyMessageLayer();


	};

	





}