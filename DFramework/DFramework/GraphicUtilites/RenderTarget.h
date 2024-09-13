#pragma once
#include "../pch.h"
#include "Texture.h"

namespace FDW
{


	class RenderTarget
	{

	public:

		RenderTarget() = delete;
		RenderTarget(ID3D12Device* pDevice, const DXGI_FORMAT format, const D3D12_RTV_DIMENSION dimension, const UINT arrSize,
			const UINT width, const UINT height,
			const DXGI_SAMPLE_DESC sampleDesc);
		~RenderTarget() = default;

		ID3D12Resource* GetRTVResource() const;
		Texture* GetTexture() const;
		D3D12_RENDER_TARGET_VIEW_DESC GetRTVDesc() const;

		void StartDraw(ID3D12GraphicsCommandList* pCommandList);
		void EndDraw(ID3D12GraphicsCommandList* pCommandList);

	private:

		void InitRTV(ID3D12Device* pDevice, const DXGI_FORMAT format, const D3D12_RTV_DIMENSION dimension,
			const UINT width, const UINT height,
			const DXGI_SAMPLE_DESC sampleDesc, const UINT arrSize);

		std::unique_ptr<Texture> rtvTexture;
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;

	};




}
