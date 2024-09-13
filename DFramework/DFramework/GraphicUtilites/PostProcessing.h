#pragma once
#include "../pch.h"

namespace FDW {


	class PostProcessing {
	public:

		PostProcessing(ID3D12Resource* texture);
		PostProcessing()=default;
		virtual ~PostProcessing()=default;

		void InverseEffect(ID3D12Device* device, bool deleteAfter=false);
		void GreyColorEffect(ID3D12Device* device, bool deleteAfter=false);
		void SharpnessColorEffect(ID3D12Device* device, bool deleteAfter=false);
		void BlurColorEffect(ID3D12Device* device, bool deleteAfter=false);

		void SetResource(ID3D12Resource* texture);

	private:

		void TextureCheckAssert();

		ID3D12Resource* pTexture;

	};




}
