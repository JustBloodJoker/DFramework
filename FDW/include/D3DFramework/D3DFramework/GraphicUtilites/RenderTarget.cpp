#include "../pch.h"
#include "RenderTarget.h"



namespace FD3DW
{


	void RenderTarget::InitRTV(ID3D12Device* pDevice, const DXGI_FORMAT format, const D3D12_RTV_DIMENSION dimension,
								const UINT width, const UINT height, 
									const DXGI_SAMPLE_DESC sampleDesc,  const UINT arrSize)
	{
        CONSOLE_MESSAGE(std::string("INITING CUSTOM RTV DESC"));

		ZeroMemory(&m_xRTVDesc, sizeof(m_xRTVDesc));

		m_xRTVDesc.Format = format;
		m_xRTVDesc.ViewDimension = dimension;

        D3D12_RESOURCE_DIMENSION textureDimension;

        switch (dimension)
        {
        case D3D12_RTV_DIMENSION_TEXTURE1D:
            m_xRTVDesc.Texture1D.MipSlice = 0;
            textureDimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
            break;

        case D3D12_RTV_DIMENSION_TEXTURE1DARRAY:
            m_xRTVDesc.Texture1DArray.MipSlice = 0;
            m_xRTVDesc.Texture1DArray.FirstArraySlice = 0;
            m_xRTVDesc.Texture1DArray.ArraySize = arrSize;
            textureDimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
            break;

        case D3D12_RTV_DIMENSION_TEXTURE2D:
            if (sampleDesc.Count == 1)
            {
                m_xRTVDesc.Texture2D.MipSlice = 0;
                m_xRTVDesc.Texture2D.PlaneSlice = 0;
                textureDimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
                break;
            }
            m_xRTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;

        case D3D12_RTV_DIMENSION_TEXTURE2DMS:
            textureDimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            break;

        case D3D12_RTV_DIMENSION_TEXTURE2DARRAY:
            if (sampleDesc.Count == 1)
            {
                m_xRTVDesc.Texture2DArray.MipSlice = 0;
                m_xRTVDesc.Texture2DArray.PlaneSlice = 0;
                m_xRTVDesc.Texture2DArray.FirstArraySlice = 0;
                m_xRTVDesc.Texture2DArray.ArraySize = arrSize;
                textureDimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
                break;
            }
            m_xRTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;

        case D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY:
            m_xRTVDesc.Texture2DMSArray.FirstArraySlice = 0;
            m_xRTVDesc.Texture2DMSArray.ArraySize = arrSize;
            textureDimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            break;

        case D3D12_RTV_DIMENSION_TEXTURE3D:
            m_xRTVDesc.Texture3D.MipSlice = 0;
            m_xRTVDesc.Texture3D.FirstWSlice = 1;
            m_xRTVDesc.Texture3D.WSize = -1;
            textureDimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
            break;

        default:
            textureDimension = D3D12_RESOURCE_DIMENSION_UNKNOWN;
            break;
        }
        
		m_pRTVTexture = std::make_unique<FResource>(pDevice, arrSize, format, width, height, sampleDesc, textureDimension, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, D3D12_TEXTURE_LAYOUT_UNKNOWN, D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES);
        
	}


	RenderTarget::RenderTarget(ID3D12Device* pDevice, const DXGI_FORMAT format, const D3D12_RTV_DIMENSION dimension, const UINT arrSize, const UINT width, const UINT height, const DXGI_SAMPLE_DESC sampleDesc)
	{
		InitRTV(pDevice, format, dimension, width, height, sampleDesc, arrSize);
	}

	ID3D12Resource* RenderTarget::GetRTVResource() const
	{
		return m_pRTVTexture->GetResource();
	}
    
    FResource* RenderTarget::GetTexture() const
    {
        return m_pRTVTexture.get();
    }

	D3D12_RENDER_TARGET_VIEW_DESC RenderTarget::GetRTVDesc() const
	{
		return m_xRTVDesc;
	}
    
    void RenderTarget::StartDraw(ID3D12GraphicsCommandList* pCommandList)
    {
        m_pRTVTexture->ResourceBarrierChange(pCommandList, 1, D3D12_RESOURCE_STATE_RENDER_TARGET);
    }

    void RenderTarget::EndDraw(ID3D12GraphicsCommandList* pCommandList)
    {
        m_pRTVTexture->ResourceBarrierChange(pCommandList, 1, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }
}
