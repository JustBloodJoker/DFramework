#pragma once 

#include "../pch.h"
#include "FResource.h"

namespace FD3DW
{


	class DepthStencilView : public FD3DW::FResource
	{

	public:

		DepthStencilView() = delete;
		DepthStencilView(ID3D12Device* pDevice, const DXGI_FORMAT format, const D3D12_DSV_DIMENSION dimension, const UINT arrSize,
			const UINT width, const UINT height,
			const DXGI_SAMPLE_DESC sampleDesc, const D3D12_DSV_FLAGS flags = D3D12_DSV_FLAG_NONE,
			UINT mipsCount=1u);
		~DepthStencilView()=default;

		D3D12_DEPTH_STENCIL_VIEW_DESC GetDSVDesc() const;

		void DepthWrite(ID3D12GraphicsCommandList* pCommandList);
		void DepthRead(ID3D12GraphicsCommandList* pCommandList);
		void SRVPass(ID3D12GraphicsCommandList* pCommandList);

	private:

		D3D12_RESOURCE_DIMENSION GetDimensionForDSV(D3D12_DSV_DIMENSION dimension);

		void InitDSV(ID3D12Device* pDevice, const DXGI_FORMAT format, const D3D12_DSV_DIMENSION dimension,
			const UINT width, const UINT height,
			const DXGI_SAMPLE_DESC sampleDesc, const UINT arrSize, const D3D12_DSV_FLAGS flags);

		D3D12_DEPTH_STENCIL_VIEW_DESC m_xDSVDesc;
	};



}