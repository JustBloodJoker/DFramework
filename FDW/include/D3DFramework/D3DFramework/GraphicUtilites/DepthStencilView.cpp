#include "../pch.h"
#include "DepthStencilView.h"


namespace FD3DW
{

	DepthStencilView::DepthStencilView(ID3D12Device* pDevice, const DXGI_FORMAT format, const D3D12_DSV_DIMENSION dimension, const UINT arrSize, const UINT width, const UINT height, const DXGI_SAMPLE_DESC sampleDesc, const D3D12_DSV_FLAGS flags, UINT mipsCount)
        : FResource(pDevice, arrSize, format, width, height, sampleDesc, GetDimensionForDSV(dimension), D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, D3D12_TEXTURE_LAYOUT_UNKNOWN, D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, &keep(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)) ,mipsCount)
	{
        InitDSV(pDevice, format, dimension, width, height, sampleDesc, arrSize, flags);
	}

    D3D12_DEPTH_STENCIL_VIEW_DESC DepthStencilView::GetDSVDesc() const
	{
		return m_xDSVDesc;
	}

    void DepthStencilView::DepthWrite(ID3D12GraphicsCommandList* pCommandList)
    {
        ResourceBarrierChange(pCommandList, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    }

    void DepthStencilView::DepthRead(ID3D12GraphicsCommandList* pCommandList)
    {
        ResourceBarrierChange(pCommandList, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_DEPTH_READ);
    }

    void DepthStencilView::SRVPass(ID3D12GraphicsCommandList* pCommandList)
    {
        ResourceBarrierChange(pCommandList, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }

    D3D12_RESOURCE_DIMENSION DepthStencilView::GetDimensionForDSV(D3D12_DSV_DIMENSION dimension)
    {
        D3D12_RESOURCE_DIMENSION ret = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_UNKNOWN;
        switch (dimension)
        {
        case D3D12_DSV_DIMENSION_TEXTURE1D:
        case D3D12_DSV_DIMENSION_TEXTURE1DARRAY:
            ret = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
            break;
        case D3D12_DSV_DIMENSION_TEXTURE2D:
        case D3D12_DSV_DIMENSION_TEXTURE2DMS:
        case D3D12_DSV_DIMENSION_TEXTURE2DARRAY:
        case D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY:
            ret = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            break;
        default:
            ret = D3D12_RESOURCE_DIMENSION_UNKNOWN;
            break;
        }

        return ret;
    }

    void DepthStencilView::InitDSV(ID3D12Device* pDevice, const DXGI_FORMAT format, const D3D12_DSV_DIMENSION dimension, const UINT width, const UINT height, const DXGI_SAMPLE_DESC sampleDesc, const UINT arrSize, const D3D12_DSV_FLAGS flags)
	{
        CONSOLE_MESSAGE(std::string("INITING CUSTOM DSV DESC"));

        ZeroMemory(&m_xDSVDesc, sizeof(m_xDSVDesc));

        m_xDSVDesc.Flags = flags;
        m_xDSVDesc.Format = format;
        m_xDSVDesc.ViewDimension = dimension;

        switch (dimension)
        {
        case D3D12_DSV_DIMENSION_TEXTURE1D:
            m_xDSVDesc.Texture1D.MipSlice = 0;
            break;

        case D3D12_DSV_DIMENSION_TEXTURE1DARRAY:
            m_xDSVDesc.Texture1DArray.MipSlice = 0;
            m_xDSVDesc.Texture1DArray.FirstArraySlice = 0;
            m_xDSVDesc.Texture1DArray.ArraySize = arrSize;
            break;

        case D3D12_DSV_DIMENSION_TEXTURE2D:
            if (sampleDesc.Count == 1)
            {
                m_xDSVDesc.Texture2D.MipSlice = 0;
                break;
            }
            m_xDSVDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;

        case D3D12_DSV_DIMENSION_TEXTURE2DMS:
            break;

        case D3D12_DSV_DIMENSION_TEXTURE2DARRAY: 
            if (sampleDesc.Count == 1)
            {
                m_xDSVDesc.Texture2DArray.MipSlice = 0;
                m_xDSVDesc.Texture2DArray.FirstArraySlice = 0;
                m_xDSVDesc.Texture2DArray.ArraySize = arrSize;
                break;
            }
            m_xDSVDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;

        case D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY:
            m_xDSVDesc.Texture2DMSArray.FirstArraySlice = 0;
            m_xDSVDesc.Texture2DMSArray.ArraySize = arrSize;
            break;
        default:
            break;
        }
	}

}
