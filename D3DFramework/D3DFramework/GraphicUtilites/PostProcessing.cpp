#include "../pch.h"
#include "PostProcessing.h"
#include "PostProcessingAlgorithms.cuh"

namespace FD3DW {


	PostProcessing::PostProcessing(ID3D12Resource* texture) : pTexture(texture) {}

	void PostProcessing::InverseEffect(ID3D12Device* device, bool deleteAfter)
	{
		TextureCheckAssert();

		InverseTexture(pTexture, device);

		if (deleteAfter)
		{
			ClearFromMap(pTexture);
		}
	}

	void PostProcessing::GreyColorEffect(ID3D12Device* device, bool deleteAfter) 
	{
		TextureCheckAssert();

		GreyEffect(pTexture, device);

		if (deleteAfter)
		{
			ClearFromMap(pTexture);
		}
	}

	void PostProcessing::SharpnessColorEffect(ID3D12Device* device, bool deleteAfter)
	{
		TextureCheckAssert();

		SharpnessEffect(pTexture, device);

		if (deleteAfter)
		{
			ClearFromMap(pTexture);
		}
	}

	void PostProcessing::BlurColorEffect(ID3D12Device* device, bool deleteAfter)
	{
		TextureCheckAssert();

		BlurEffect(pTexture, device);

		if (deleteAfter)
		{
			ClearFromMap(pTexture);
		}
	}

	void PostProcessing::SetResource(ID3D12Resource* texture)
	{
		pTexture = texture;
	}

	void PostProcessing::TextureCheckAssert()
	{
		if (!pTexture)
		{
			CONSOLE_ERROR_MESSAGE("NOT HAVE TEXTURE TO USE POST PROCESSING!");
			SAFE_ASSERT(true, "NOT HAVE TEXTURE TO USE POST PROCESSING!");
		}
	}
}