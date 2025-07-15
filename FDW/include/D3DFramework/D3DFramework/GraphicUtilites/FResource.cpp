#include "../pch.h"
#include "FResource.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include "PipelineObject.h"
#include "TextureLoaders.h"

namespace FD3DW
{
    ///////////////////
    // COMPUTE PSO MIP GENERATOR 

    static const std::wstring s_wsMipMapGenCS = LR"(
        Texture2D<float4> SrcTexture : register(t0);
        RWTexture2D<float4> DstTexture : register(u0);
        SamplerState BilinearClamp : register(s0);
        cbuffer CB : register(b0)
        {
            float2 TexelSize;
        }

        [RootSignature(
            "RootFlags(0), \
                DescriptorTable(SRV(t0)), \
                DescriptorTable(UAV(u0)), \
                RootConstants(num32BitConstants=2, b0), \
                StaticSampler(s0, \
                filter = FILTER_MIN_MAG_LINEAR_MIP_POINT, \
                addressU = TEXTURE_ADDRESS_CLAMP, \
                addressV = TEXTURE_ADDRESS_CLAMP, \
                addressW = TEXTURE_ADDRESS_CLAMP),")]
        [numthreads(8, 8, 1)]
        void GenerateMipMaps(uint3 DTid : SV_DispatchThreadID)
        {
            float2 texcoords = TexelSize * (DTid.xy + 0.5);
            float4 color = SrcTexture.SampleLevel(BilinearClamp, texcoords, 0);
            DstTexture[DTid.xy] = color;
        }
    )";


    PipelineObject* FResource::GetMipGenerationPSO(ID3D12Device* device) {
        static std::unique_ptr<PipelineObject> s_pGenerateMipsPSO = nullptr;

        if (!s_pGenerateMipsPSO) {
            s_pGenerateMipsPSO = std::make_unique<PipelineObject>(device);
            std::unordered_map<CompileFileType, CompileDesc> shaders;
            shaders[CompileFileType::CS] = { s_wsMipMapGenCS, L"GenerateMipMaps",L"cs_6_5", false };
            s_pGenerateMipsPSO->CreatePSO(shaders);
        }

        return s_pGenerateMipsPSO.get();
    }
    ///////////////////

    std::unordered_map<std::string, std::weak_ptr<FResource>> FResource::s_vTextures;

	FResource::FResource(std::string path, ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList)
	{
        m_sPath = path;
        auto it = s_vTextures.find(path);
        if (it != s_vTextures.end())
        {
            CONSOLE_MESSAGE(std::string("TEXTURE WITH PATH: " + path + " ALREADY EXIST"));
        }
        else
        {
            CONSOLE_MESSAGE(std::string("CREATING TEXTURE WITH PATH: " + path));
            std::filesystem::path pt(path);
            std::string extension = pt.extension().string();
            if (extension == ".dds")
            {
                TextureDataOutput resultData;
                HRESULT_ASSERT(DDSTextureLoaderDX12::Load(path, pDevice, pCommandList, resultData), "Cant create dds resource");

                m_pResource = resultData.TextureResource;
                m_pUploadBuffer = std::move(resultData.ResultUploadBuffer);
            }
            else if (extension == ".hdr")
            {
                SAFE_ASSERT(false, "hdr texture parser not impl");
            }
            else if (extension == ".tga")
            {
                SAFE_ASSERT(false, "tga texture parser not impl");
            }
            else
            {
                int width, height, channels;
                float* dat = stbi_loadf(path.c_str(), &width, &height, &channels, 0);
                
                UINT mipLevels = CalculateMipCount(width, height);
                CreateTextureBuffer(pDevice, 1, 
                    channels == 1 ? DXGI_FORMAT_R32_FLOAT : channels == 2 ? DXGI_FORMAT_R32G32_FLOAT : channels == 3 ? DXGI_FORMAT_R32G32B32_FLOAT : DXGI_FORMAT_R32G32B32A32_FLOAT,
                    width, height, 
                    DXGI_SAMPLE_DESC({1, 0}),
                    D3D12_RESOURCE_DIMENSION_TEXTURE2D, 
                    D3D12_RESOURCE_FLAG_NONE, 
                    D3D12_TEXTURE_LAYOUT_UNKNOWN, 
                    D3D12_HEAP_FLAG_NONE, 
                    &keep(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)), mipLevels);

                UploadData(pDevice, pCommandList, dat);
                GenerateMips(pDevice, pCommandList);

                delete dat;
            }

        }
	}

    std::shared_ptr<FResource> FResource::GetSharedFromThis()
    {
        auto ret = shared_from_this();
        if (s_vTextures.find(m_sPath) == s_vTextures.end()) 
        {
            s_vTextures.emplace(m_sPath, std::weak_ptr<FResource>(ret));
        }

        return ret;
    }

    std::unique_ptr<FResource> FResource::CreateAnonimTexture(ID3D12Device* pDevice, const UINT16 arraySize, const DXGI_FORMAT format, const UINT width, const UINT height, DXGI_SAMPLE_DESC sampleDesc, const D3D12_RESOURCE_DIMENSION dimension, const D3D12_RESOURCE_FLAGS resourceFlags, const D3D12_TEXTURE_LAYOUT layout, const D3D12_HEAP_FLAGS heapFlags, const D3D12_HEAP_PROPERTIES* heapProperties, const UINT16 mipLevels)
    {
        return std::make_unique<FResource>(pDevice, arraySize, format, width, height, sampleDesc, dimension, resourceFlags, layout, heapFlags, heapProperties, mipLevels);
    }

    std::unique_ptr<FResource> FResource::CreateSimpleStructuredBuffer(ID3D12Device* pDevice, const UINT width)
    {
        return FResource::CreateAnonimTexture(pDevice, 1u, DXGI_FORMAT_UNKNOWN, width, 1, DXGI_SAMPLE_DESC({1,0}), D3D12_RESOURCE_DIMENSION_BUFFER, D3D12_RESOURCE_FLAG_NONE, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, &keep(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)), 1);
    }

    std::shared_ptr<FResource> FResource::CreateTextureFromPath(std::string path, ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList)
    {
        auto iter = s_vTextures.find(path);
        std::shared_ptr<FResource> ptr;
        if (iter != s_vTextures.end())
        {
            if (ptr = iter->second.lock())
                return ptr;
            else
                s_vTextures.erase(iter);
        }
        ptr = std::shared_ptr<FResource>(new FResource(path, pDevice, pCommandList));
        s_vTextures.emplace(path, std::weak_ptr<FResource>(ptr));
        return ptr;
    }

    FResource::FResource(ID3D12Device* pDevice, const UINT16 arraySize, const DXGI_FORMAT format, const UINT width, const UINT height, DXGI_SAMPLE_DESC sampleDesc, const D3D12_RESOURCE_DIMENSION dimension, const D3D12_RESOURCE_FLAGS resourceFlags, const D3D12_TEXTURE_LAYOUT layout, const D3D12_HEAP_FLAGS heapFlags, const D3D12_HEAP_PROPERTIES* heapProperties, const UINT16 mipLevels)
    {
        CONSOLE_MESSAGE(std::string("CREATING ANONIM TEXTURE"));

        CreateTextureBuffer(pDevice, arraySize, format, width, height, sampleDesc, dimension, resourceFlags, layout, heapFlags, heapProperties, mipLevels);
    }

	FResource::~FResource()
	{
	}

    UINT FResource::CalculateMipCount(UINT width, UINT height)
    {
        return 1 + static_cast<UINT>(std::floor(std::log2(std::max(width, height))));
    }

    bool FResource::IsSupportMipMapping(D3D12_RESOURCE_DESC desc)
    {
        return desc.Format == DXGI_FORMAT_R32G32B32A32_FLOAT;
    }


    void FResource::CreateTextureBuffer(ID3D12Device* pDevice, const UINT16 arraySize, const DXGI_FORMAT format, const UINT width, const UINT height, DXGI_SAMPLE_DESC sampleDesc, const D3D12_RESOURCE_DIMENSION dimension, const D3D12_RESOURCE_FLAGS resourceFlags, const D3D12_TEXTURE_LAYOUT layout, const D3D12_HEAP_FLAGS heapFlags, const D3D12_HEAP_PROPERTIES* heapProperties, const UINT16 mipLevels, bool willGenerateMips)
    {
        D3D12_RESOURCE_DESC txtDesc = {};
        

        txtDesc.MipLevels = mipLevels;
        txtDesc.DepthOrArraySize = arraySize;
        txtDesc.Format = format;
        txtDesc.Width = width;
        txtDesc.Height = height;
        txtDesc.SampleDesc = sampleDesc;
        txtDesc.Dimension = dimension;
        txtDesc.Flags = resourceFlags;
        txtDesc.Layout = layout;

        if (willGenerateMips) {
            txtDesc.MipLevels = IsSupportMipMapping(txtDesc) ? txtDesc.MipLevels : 1;
            if (txtDesc.MipLevels > 1) {
                txtDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;//for generating mip levels
            }

        }

        HRESULT_ASSERT(pDevice->CreateCommittedResource(
            heapProperties,
            heapFlags | D3D12_HEAP_FLAG_SHARED, //for post processing
            &txtDesc,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(m_pResource.GetAddressOf())), " CREATE TEXTURE RESOURCE ERROR");

        SetResource(m_pResource.Get());

        m_xCurrState = D3D12_RESOURCE_STATE_COMMON;
    }


    void FResource::UploadData(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, const void* pData, D3D12_RESOURCE_STATES state,bool bCopyAllMips)
    {
        auto uresource = m_pResource.Get();
        auto desc = uresource->GetDesc();

        auto arrSize = desc.DepthOrArraySize;
        auto mipLevels = bCopyAllMips ? desc.MipLevels : 1;

        size_t expectedSubresources = static_cast<size_t>(mipLevels) * arrSize;

        UINT64 totalBytes = GetRequiredIntermediateSize(uresource, 0, expectedSubresources);

        std::vector<D3D12_SUBRESOURCE_DATA> subResData;
        subResData.reserve(expectedSubresources);
        subResData.resize(expectedSubresources);

        auto ptrData = reinterpret_cast<const uint8_t*>(pData);
        for (UINT arrayIdx = 0; arrayIdx < arrSize; ++arrayIdx)
        {
            UINT w = desc.Width;
            UINT h = desc.Height;

            for (UINT mipIdx = 0; mipIdx < mipLevels; ++mipIdx)
            {
                UINT subresourceIndex = mipIdx + arrayIdx * mipLevels;

                D3D12_SUBRESOURCE_DATA textureData = {};
                textureData.pData = ptrData;
                textureData.RowPitch = w * GetFormatSizeInBytes(desc.Format);
                textureData.SlicePitch = textureData.RowPitch * h;

                subResData[subresourceIndex] = textureData;

                ptrData += textureData.SlicePitch;
               
                w = std::max(1u, w >> 1);
                h = std::max(1u, h >> 1);
            }
        }

        if (!m_pUploadBuffer)
        {
            m_pUploadBuffer = std::make_unique<UploadBuffer<char>>(pDevice, static_cast<UINT>(totalBytes), false);
        }

        ResourceBarrierChange(pCommandList, 1, D3D12_RESOURCE_STATE_COPY_DEST);

        UpdateSubresources(pCommandList,
            m_pResource.Get(),
            m_pUploadBuffer->GetResource(),
            0, 0, (UINT)subResData.size(),
            subResData.data());

        ResourceBarrierChange(pCommandList, 1, state);
    }

    void FResource::GenerateMips(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList)
    {
        auto desc = m_pResource->GetDesc();
        if (desc.MipLevels <= 1) return;

        UINT numMips = desc.MipLevels;
        UINT numDescriptors = 2 * (numMips - 1);

        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.NumDescriptors = numDescriptors;
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        
        ID3D12DescriptorHeap* descriptorHeap;
        HRESULT_ASSERT(pDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&descriptorHeap)), "Failed to create descriptor heap for mipmap generation");

        UINT descriptorSize = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        
        auto mipGenPSO = GetMipGenerationPSO(pDevice);

        CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(descriptorHeap->GetCPUDescriptorHandleForHeapStart());
        CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(descriptorHeap->GetGPUDescriptorHandleForHeapStart());
        for (UINT topMip = 0; topMip < numMips - 1; ++topMip)
        {

            UINT srcSubresource = D3D12CalcSubresource(topMip, 0, 0, numMips, 1);
            UINT dstSubresource = D3D12CalcSubresource(topMip + 1, 0, 0, numMips, 1);

            D3D12_RESOURCE_BARRIER setBarriers[] = {
                CD3DX12_RESOURCE_BARRIER::Transition(m_pResource.Get(),
                    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                    D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                    srcSubresource),
                CD3DX12_RESOURCE_BARRIER::Transition(m_pResource.Get(),
                    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                    D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                    dstSubresource)
            };

            pCommandList->ResourceBarrier(ARRAYSIZE(setBarriers), setBarriers);

            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = desc.Format;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Texture2D.MostDetailedMip = topMip;
            srvDesc.Texture2D.MipLevels = 1;
            srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
            pDevice->CreateShaderResourceView(m_pResource.Get(), &srvDesc, cpuHandle);

            cpuHandle.Offset(1, descriptorSize);

            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
            uavDesc.Format = desc.Format;
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            uavDesc.Texture2D.MipSlice = topMip + 1;
            uavDesc.Texture2D.PlaneSlice = 0;
            pDevice->CreateUnorderedAccessView(m_pResource.Get(), nullptr, &uavDesc, cpuHandle);

            mipGenPSO->Bind(pCommandList);

            ID3D12DescriptorHeap* heaps[] = { descriptorHeap };
            pCommandList->SetDescriptorHeaps(1, heaps);

            uint32_t dstWidth = std::max( (UINT)desc.Width >> (topMip + 1), 1u );
            uint32_t dstHeight = std::max( (UINT)desc.Height >> (topMip + 1), 1u );
            float texelSize[2] = { 1.0f / dstWidth, 1.0f / dstHeight };

            pCommandList->SetComputeRoot32BitConstants(2, 2, texelSize, 0);
            pCommandList->SetComputeRootDescriptorTable(0, gpuHandle);
            gpuHandle.Offset(1, descriptorSize);
            pCommandList->SetComputeRootDescriptorTable(1, gpuHandle);


            pCommandList->Dispatch((dstWidth + 7) / 8, (dstHeight + 7) / 8, 1);
            
            D3D12_RESOURCE_BARRIER resetBarriers[] = {
                CD3DX12_RESOURCE_BARRIER::Transition(m_pResource.Get(),
                    D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                    srcSubresource),
                CD3DX12_RESOURCE_BARRIER::Transition(m_pResource.Get(),
                    D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                    dstSubresource)
            };
            pCommandList->ResourceBarrier(ARRAYSIZE(resetBarriers), resetBarriers);

            cpuHandle.Offset(1, descriptorSize);
            gpuHandle.Offset(1, descriptorSize);

        }
    }

    bool FResource::DeleteUploadBuffer()
    {
        m_pUploadBuffer.reset();
        return !m_pUploadBuffer;
    }
    
    void FResource::ResourceBarrierChange(ID3D12GraphicsCommandList* pCommandList, const UINT numBariers, const D3D12_RESOURCE_STATES resourceStateAfter)
    {
        if (m_xCurrState != resourceStateAfter)
        {
            pCommandList->ResourceBarrier(1,
                &keep(CD3DX12_RESOURCE_BARRIER::Transition(m_pResource.Get(),
                    m_xCurrState,
                    resourceStateAfter)));

            m_xCurrState = resourceStateAfter;
        }
    }

    ID3D12Resource* FResource::GetResource() const
    {
        return m_pResource.Get();
    }

    void FResource::ReleaseUploadBuffers() 
    {
        for (auto& el : FResource::s_vTextures)
        {
            auto ptr = el.second.lock();
            if (ptr && ptr->m_pUploadBuffer) 
            {
                auto h = ptr->m_pUploadBuffer.release();
                delete h;
            }
        }
    }

}