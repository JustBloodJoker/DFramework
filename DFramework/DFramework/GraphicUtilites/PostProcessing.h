#pragma once
#include "../pch.h"

namespace FDW {


	class PostProcessing {
	public:

		PostProcessing(ID3D12Resource* texture);
		PostProcessing()=delete;
		~PostProcessing()=default;

		void InverseEffect(ID3D12Device* device, bool deleteAfter=false);

		void SaveTexture();

	private:

		ID3D12Resource* pTexture;

	};




}
