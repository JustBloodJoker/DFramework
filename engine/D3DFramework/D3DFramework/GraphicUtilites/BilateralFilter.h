#pragma once

#include "../pch.h"
#include "ResourcePacker.h"
#include "FResource.h"
#include "ComputePipelineObject.h"
#include <stb_image.h>

namespace FD3DW {


	struct BilateralParams
	{
		dx::XMFLOAT2 TexelSize;
		float SigmaS = 2.0f;
		float SigmaR = 0.2f;
		int KernelRadius = 5;
	};

	class BilateralFilter {
	public:
		static void TryCreateBilateralFilterPSO(ID3D12Device* device);
		static ComputePipelineObject* GetBilateralFilterPSO();

	public:
		BilateralFilter(ID3D12Device* device, FResource* resource);
		BilateralFilter(ID3D12Device* device, FResource* resource, BilateralParams params);
		~BilateralFilter() = default;

		void SetResource(ID3D12Device* device, FResource* resource);
		void SetParams(BilateralParams params);
		
		void Apply(ID3D12GraphicsCommandList* list);

		FResource* GetDstResource();
		BilateralParams GetParams();

	protected:
		void UpdateParamsBuffer();

	protected:
		FResource* m_pSrcResource;
		std::unique_ptr<FResource> m_pDstResource;

		BilateralParams m_xParams;
		std::unique_ptr<SRV_UAVPacker> m_pPack;
		std::unique_ptr<UploadBuffer<BilateralParams>> m_pParamsBuffer;

	};


};
