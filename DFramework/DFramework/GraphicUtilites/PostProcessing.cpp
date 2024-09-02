#include "../pch.h"
#include "PostProcessing.h"
#include "PostProcessingAlgorithms.cuh"

namespace FDW {


	PostProcessing::PostProcessing(ID3D12Resource* texture) : pTexture(texture) {}

	void PostProcessing::InverseEffect(ID3D12Device* device, bool deleteAfter) {
		InverseTexture(pTexture, device);

		if (deleteAfter)
		{
			ClearFromMap(pTexture);
		}
	}

	void PostProcessing::SaveTexture() {


	}

}