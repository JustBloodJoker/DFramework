#include "../pch.h"
#include "PostProcessing.h"
#include <D3DFramework/GraphicUtilites/PostProcessingAlgorithms.cuh>

namespace FD3DW {


	PostProcessing::PostProcessing(ID3D12Resource* texture) : m_pTexture(texture) {}

	void PostProcessing::InverseEffect(ID3D12Device* device, bool deleteAfter)
	{
		TextureCheckAssert();

		InverseTexture(m_pTexture, device);

		if (deleteAfter)
		{
			ClearFromMap(m_pTexture);
		}
	}

	void PostProcessing::GreyColorEffect(ID3D12Device* device, bool deleteAfter) 
	{
		TextureCheckAssert();

		GreyEffect(m_pTexture, device);

		if (deleteAfter)
		{
			ClearFromMap(m_pTexture);
		}
	}

	void PostProcessing::SharpnessColorEffect(ID3D12Device* device, bool deleteAfter)
	{
		TextureCheckAssert();

		SharpnessEffect(m_pTexture, device);

		if (deleteAfter)
		{
			ClearFromMap(m_pTexture);
		}
	}

	void PostProcessing::BlurColorEffect(ID3D12Device* device, bool deleteAfter)
	{
		TextureCheckAssert();

		BlurEffect(m_pTexture, device);

		if (deleteAfter)
		{
			ClearFromMap(m_pTexture);
		}
	}

	void PostProcessing::SetResource(ID3D12Resource* texture)
	{
		m_pTexture = texture;
	}

	void PostProcessing::TextureCheckAssert()
	{
		if (!m_pTexture)
		{
			CONSOLE_ERROR_MESSAGE("NOT HAVE TEXTURE TO USE POST PROCESSING!");
			SAFE_ASSERT(true, "NOT HAVE TEXTURE TO USE POST PROCESSING!");
		}
	}
}