#include "../pch.h"
#include "DepthStencilView.h"


namespace FD3DW
{

	DepthStencilView::DepthStencilView(ID3D12Device* pDevice, const DXGI_FORMAT format, const D3D12_DSV_DIMENSION dimension, const UINT arrSize, const UINT width, const UINT height, const DXGI_SAMPLE_DESC sampleDesc, const D3D12_DSV_FLAGS flags)
	{
        InitDSV(pDevice, format, dimension, width, height, sampleDesc, arrSize, flags);
	}

	ID3D12Resource* DepthStencilView::GetDSVResource() const
	{
		return dsvTexture->GetResource();
	}

    D3D12_DEPTH_STENCIL_VIEW_DESC DepthStencilView::GetDSVDesc() const
	{
		return dsvDesc;
	}

    void DepthStencilView::DepthWrite(ID3D12GraphicsCommandList* pCommandList)
    {
        dsvTexture->ResourceBarrierChange(pCommandList, 1, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    }

    void DepthStencilView::DepthRead(ID3D12GraphicsCommandList* pCommandList)
    {
        dsvTexture->ResourceBarrierChange(pCommandList, 1, D3D12_RESOURCE_STATE_DEPTH_READ);
    }

    void DepthStencilView::SRVPass(ID3D12GraphicsCommandList* pCommandList)
    {
        dsvTexture->ResourceBarrierChange(pCommandList, 1, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }

	void DepthStencilView::InitDSV(ID3D12Device* pDevice, const DXGI_FORMAT format, const D3D12_DSV_DIMENSION dimension, const UINT width, const UINT height, const DXGI_SAMPLE_DESC sampleDesc, const UINT arrSize, const D3D12_DSV_FLAGS flags)
	{
        CONSOLE_MESSAGE(std::string("INITING CUSTOM DSV DESC"));

        ZeroMemory(&dsvDesc, sizeof(dsvDesc));

        dsvDesc.Flags = flags;
        dsvDesc.Format = format;
        dsvDesc.ViewDimension = dimension;

        D3D12_RESOURCE_DIMENSION textureDimension;

        switch (dimension)
        {
        case D3D12_DSV_DIMENSION_TEXTURE1D:
            dsvDesc.Texture1D.MipSlice = 0;
            textureDimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
            break;

        case D3D12_DSV_DIMENSION_TEXTURE1DARRAY:
            dsvDesc.Texture1DArray.MipSlice = 0;
            dsvDesc.Texture1DArray.FirstArraySlice = 0;
            dsvDesc.Texture1DArray.ArraySize = arrSize;
            textureDimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
            break;

        case D3D12_DSV_DIMENSION_TEXTURE2D:
            if (sampleDesc.Count == 1)
            {
                dsvDesc.Texture2D.MipSlice = 0;
                textureDimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
                break;
            }
            dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;

        case D3D12_DSV_DIMENSION_TEXTURE2DMS:
            textureDimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            break;

        case D3D12_DSV_DIMENSION_TEXTURE2DARRAY: 
            if (sampleDesc.Count == 1)
            {
                dsvDesc.Texture2DArray.MipSlice = 0;
                dsvDesc.Texture2DArray.FirstArraySlice = 0;
                dsvDesc.Texture2DArray.ArraySize = arrSize;
                textureDimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
                break;
            }
            dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;

        case D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY:
            dsvDesc.Texture2DMSArray.FirstArraySlice = 0;
            dsvDesc.Texture2DMSArray.ArraySize = arrSize;
            textureDimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            break;
        default:
            
            textureDimension = D3D12_RESOURCE_DIMENSION_UNKNOWN;
            break;
        }
        dsvTexture = std::make_unique<FResource>(pDevice, arrSize, format, width, height, sampleDesc, textureDimension, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, D3D12_TEXTURE_LAYOUT_UNKNOWN, D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES);

	}

}
