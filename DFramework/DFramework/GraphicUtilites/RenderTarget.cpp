#include "../pch.h"
#include "RenderTarget.h"



namespace FDW
{


	void RenderTarget::InitRTV(ID3D12Device* pDevice, const DXGI_FORMAT format, const D3D12_RTV_DIMENSION dimension,
								const UINT width, const UINT height, 
									const DXGI_SAMPLE_DESC sampleDesc,  const UINT arrSize)
	{
        CONSOLE_MESSAGE(std::string("INITING CUSTOM RTV DESC"));

		ZeroMemory(&rtvDesc, sizeof(rtvDesc));

		rtvDesc.Format = format;
		rtvDesc.ViewDimension = dimension;

        D3D12_RESOURCE_DIMENSION textureDimension;

        switch (dimension)
        {
        case D3D12_RTV_DIMENSION_TEXTURE1D:
            rtvDesc.Texture1D.MipSlice = 0;
            textureDimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
            break;

        case D3D12_RTV_DIMENSION_TEXTURE1DARRAY:
            rtvDesc.Texture1DArray.MipSlice = 0;
            rtvDesc.Texture1DArray.FirstArraySlice = 0;
            rtvDesc.Texture1DArray.ArraySize = arrSize;
            textureDimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
            break;

        case D3D12_RTV_DIMENSION_TEXTURE2D:
            if (sampleDesc.Count == 1)
            {
                rtvDesc.Texture2D.MipSlice = 0;
                rtvDesc.Texture2D.PlaneSlice = 0;
                textureDimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
                break;
            }
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;

        case D3D12_RTV_DIMENSION_TEXTURE2DMS:
            textureDimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            break;

        case D3D12_RTV_DIMENSION_TEXTURE2DARRAY:
            if (sampleDesc.Count == 1)
            {
                rtvDesc.Texture2DArray.MipSlice = 0;
                rtvDesc.Texture2DArray.PlaneSlice = 0;
                rtvDesc.Texture2DArray.FirstArraySlice = 0;
                rtvDesc.Texture2DArray.ArraySize = arrSize;
                textureDimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
                break;
            }
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;

        case D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY:
            rtvDesc.Texture2DMSArray.FirstArraySlice = 0;
            rtvDesc.Texture2DMSArray.ArraySize = arrSize;
            textureDimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            break;

        case D3D12_RTV_DIMENSION_TEXTURE3D:
            rtvDesc.Texture3D.MipSlice = 0;
            rtvDesc.Texture3D.FirstWSlice = 1;
            rtvDesc.Texture3D.WSize = -1;
            textureDimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
            break;

        default:
            textureDimension = D3D12_RESOURCE_DIMENSION_UNKNOWN;
            break;
        }
		rtvTexture = std::make_unique<Texture>(pDevice, arrSize, format, width, height, sampleDesc, textureDimension, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, D3D12_TEXTURE_LAYOUT_UNKNOWN, D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES);
        
	}


	RenderTarget::RenderTarget(ID3D12Device* pDevice, const DXGI_FORMAT format, const D3D12_RTV_DIMENSION dimension, const UINT arrSize, const UINT width, const UINT height, const DXGI_SAMPLE_DESC sampleDesc)
	{
		InitRTV(pDevice, format, dimension, width, height, sampleDesc, arrSize);
	}

	ID3D12Resource* RenderTarget::GetRTVResource() const
	{
		return rtvTexture->GetResource();
	}
    
    Texture* RenderTarget::GetTexture() const
    {
        return rtvTexture.get();
    }

	D3D12_RENDER_TARGET_VIEW_DESC RenderTarget::GetRTVDesc() const
	{
		return rtvDesc;
	}
    
    void RenderTarget::StartDraw(ID3D12GraphicsCommandList* pCommandList)
    {
        rtvTexture->ResourceBarrierChange(pCommandList, 1, D3D12_RESOURCE_STATE_RENDER_TARGET);
    }

    void RenderTarget::EndDraw(ID3D12GraphicsCommandList* pCommandList)
    {
        rtvTexture->ResourceBarrierChange(pCommandList, 1, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }
}
