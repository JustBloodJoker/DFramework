#include "BilateralFilter.h"
#include "GraphicsPipelineObject.h"


namespace FD3DW {

        static const std::wstring s_wsBilateralFilterCS = LR"(
        Texture2D<float> SrcTexture : register(t0);
        RWTexture2D<float> DstTexture : register(u0);

        SamplerState PointClamp : register(s0);
    
        struct BilateralFilterParams {
            float2 TexelSize;
            float  SigmaS;
            float  SigmaR;
            int    KernelRadius;
        };

        ConstantBuffer<BilateralFilterParams> BilateralParams : register(b0);
        

        [RootSignature(
        "RootFlags(0), \
            DescriptorTable(SRV(t0)), \
            DescriptorTable(UAV(u0)), \
            CBV(b0), \
            StaticSampler(s0, \
                filter = FILTER_MIN_MAG_MIP_POINT, \
                addressU = TEXTURE_ADDRESS_CLAMP, \
                addressV = TEXTURE_ADDRESS_CLAMP, \
                addressW = TEXTURE_ADDRESS_CLAMP),"
            )]
        [numthreads(8, 8, 1)]
        void BilateralFilter(uint3 DTid : SV_DispatchThreadID)
        {
            uint2 coord = DTid.xy;
            float centerValue = SrcTexture[coord];

            float sumWeights = 0.0;
            float sumValues = 0.0;

            for (int j = -BilateralParams.KernelRadius; j <= BilateralParams.KernelRadius; ++j)
            {
                for (int i = -BilateralParams.KernelRadius; i <= BilateralParams.KernelRadius; ++i)
                {
                    uint2 sampleCoord = coord + uint2(i, j);

                    float sampleValue = SrcTexture[sampleCoord];

                    float2 offset = float2(i, j) * BilateralParams.TexelSize;
                    float spatialWeight = exp(-dot(offset, offset) / (2.0 * BilateralParams.SigmaS * BilateralParams.SigmaS));

                    float diff = sampleValue - centerValue;
                    float rangeWeight = exp(-(diff * diff) / (2.0 * BilateralParams.SigmaR * BilateralParams.SigmaR));

                    float weight = spatialWeight * rangeWeight;

                    sumWeights += weight;
                    sumValues += sampleValue * weight;
                }
            }

            DstTexture[coord] = sumValues / sumWeights;
        }
    )";

    static std::unique_ptr<ComputePipelineObject> s_pBilateralFilterPSO = nullptr;


    void BilateralFilter::TryCreateBilateralFilterPSO(ID3D12Device* device) {
        if (!s_pBilateralFilterPSO) {
            s_pBilateralFilterPSO = std::make_unique<ComputePipelineObject>(device);
            std::unordered_map<CompileFileType, CompileDesc> shaders;
            shaders[CompileFileType::CS] = { s_wsBilateralFilterCS, L"BilateralFilter",L"cs_6_5", false };
            s_pBilateralFilterPSO->CreatePSO(shaders);
        }
    }


    ComputePipelineObject* BilateralFilter::GetBilateralFilterPSO()
    {
        return s_pBilateralFilterPSO.get();
    }

    BilateralFilter::BilateralFilter(ID3D12Device* device, FResource* resource) : BilateralFilter(device, resource, {}) {}

	BilateralFilter::BilateralFilter(ID3D12Device* device, FResource* resource, BilateralParams params) {
        TryCreateBilateralFilterPSO(device);
        
        m_pParamsBuffer = FD3DW::UploadBuffer<BilateralParams>::CreateConstantBuffer(device, 1);
		auto size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pPack = FD3DW::SRV_UAVPacker::CreatePack(size, 2u, 0, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, device);

		SetResource(device, resource);
		SetParams(params);
	}

	void BilateralFilter::SetResource(ID3D12Device* device, FResource* resource)
	{
		auto desc = resource->GetResource()->GetDesc();
		SAFE_ASSERT((desc.Dimension==D3D12_RESOURCE_DIMENSION_TEXTURE2D), "BilateralFilter can works only with texture 2d");

		m_pSrcResource = resource;
		m_pPack->AddResource(m_pSrcResource->GetResource(), D3D12_SRV_DIMENSION_TEXTURE2D, 0, device);

        m_pDstResource = m_pSrcResource->MakeCopy(device);

        FD3DW::UAVResourceDesc descUav;
        descUav.Resource = m_pDstResource->GetResource();
        descUav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        descUav.Format = m_pDstResource->GetResource()->GetDesc().Format;
        descUav.MipSlice = 0;
        descUav.PlaneSlice = 0;
        m_pPack->AddResource(descUav, 1, device);
	}

	void BilateralFilter::SetParams(BilateralParams params)
	{
		m_xParams = params;
		UpdateParamsBuffer();
	}

    void BilateralFilter::Apply(ID3D12GraphicsCommandList* list)
    {
        GetBilateralFilterPSO()->Bind(list);

        m_pSrcResource->ResourceBarrierChange(list, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        m_pDstResource->ResourceBarrierChange(list, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        list->SetComputeRootConstantBufferView(2, m_pParamsBuffer->GetGPULocation(0));

        ID3D12DescriptorHeap* heaps[] = { m_pPack->GetResult()->GetDescriptorPtr() };
        list->SetDescriptorHeaps(ARRAYSIZE(heaps), heaps);

        list->SetComputeRootDescriptorTable(0, m_pPack->GetResult()->GetGPUDescriptorHandle(0));
        list->SetComputeRootDescriptorTable(1,m_pPack->GetResult()->GetGPUDescriptorHandle(1));

        auto texDesc = m_pSrcResource->GetResource()->GetDesc();

        UINT groupsX = (UINT)ceilf(texDesc.Width / 8.0f);
        UINT groupsY = (UINT)ceilf(texDesc.Height / 8.0f);

        list->Dispatch(groupsX, groupsY, 1);

        m_pSrcResource->ResourceBarrierChange(list, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        m_pDstResource->ResourceBarrierChange(list, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }

    FResource* BilateralFilter::GetDstResource()
    {
        return m_pDstResource.get();
    }

    BilateralParams BilateralFilter::GetParams()
    {
        return m_xParams;
    }

	void BilateralFilter::UpdateParamsBuffer()
	{
		auto desc = m_pSrcResource->GetResource()->GetDesc();
		m_xParams.TexelSize = { 1.0f/ desc.Width , 1.0f / desc.Height };
		m_pParamsBuffer->CpyData(0, m_xParams);
	}


}
