#include "../pch.h"
#include "FResource.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace FD3DW
{
    std::unordered_map<std::string, std::weak_ptr<FResource>> FResource::textures;

	FResource::FResource(std::string path, ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList)
	{
        Path = path;
        auto it = textures.find(path);
        if (it != textures.end())
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
            }
            else if (extension == ".tga")
            {
            }
            else
            {
                int width, height, channels;
                float* dat = stbi_loadf(path.c_str(), &width, &height, &channels, 0);
                    
                CreateTextureBuffer(pDevice, 1, channels == 1 ? DXGI_FORMAT_R32_FLOAT : channels == 2 ? DXGI_FORMAT_R32G32_FLOAT : channels == 3 ? DXGI_FORMAT_R32G32B32_FLOAT : DXGI_FORMAT_R32G32B32A32_FLOAT, width, height, DXGI_SAMPLE_DESC({1, 0}),D3D12_RESOURCE_DIMENSION_TEXTURE2D);
                UploadData(pDevice, pCommandList, dat, true);

                delete dat;
            }

        }
	}

    std::shared_ptr<FResource> FResource::GetSharedFromThis()
    {
        auto ret = shared_from_this();
        if (textures.find(Path) == textures.end()) 
        {
            textures.emplace(Path, std::weak_ptr<FResource>(ret));
        }

        return ret;
    }

    std::shared_ptr<FResource> FResource::CreateTextureFromPath(std::string path, ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList)
    {
        auto iter = textures.find(path);
        std::shared_ptr<FResource> ptr;
        if (iter != textures.end())
        {
            if (ptr = iter->second.lock())
                return ptr;
            else
                textures.erase(iter);
        }
        ptr = std::shared_ptr<FResource>(new FResource(path, pDevice, pCommandList));
        textures.emplace(path, std::weak_ptr<FResource>(ptr));
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

    void FResource::CreateTextureBuffer(ID3D12Device* pDevice, const UINT16 arraySize, const DXGI_FORMAT format, const UINT width, const UINT height, DXGI_SAMPLE_DESC sampleDesc, const D3D12_RESOURCE_DIMENSION dimension, const D3D12_RESOURCE_FLAGS resourceFlags, const D3D12_TEXTURE_LAYOUT layout, const D3D12_HEAP_FLAGS heapFlags, const D3D12_HEAP_PROPERTIES* heapProperties, const UINT16 mipLevels)
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
        
        HRESULT_ASSERT(pDevice->CreateCommittedResource(
            heapProperties,
            heapFlags | D3D12_HEAP_FLAG_SHARED, //for post processing
            &txtDesc,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(pResource.GetAddressOf())), " CREATE TEXTURE RESOURCE ERROR");

        SetResource(pResource.Get());

        currState = D3D12_RESOURCE_STATE_COMMON;
    }

    void FResource::UploadData(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, const void* pData, bool checkCalculation)
    {
        auto uresource = pResource.Get();
        auto uresourceDesc = uresource->GetDesc();
        UINT64 uploadBufferSize = GetRequiredIntermediateSize(uresource, 0, uresourceDesc.DepthOrArraySize * uresourceDesc.MipLevels);
        if (checkCalculation) 
        {
            CONSOLE_MESSAGE("CHECKING UPLOAD BUFFER SIZE CALCULATIONS!!");
            UINT64 myCalculationsSize = uresourceDesc.DepthOrArraySize * uresourceDesc.MipLevels * uresourceDesc.Height * uresourceDesc.Width * sizeof(float) * GetChannelsCount(uresourceDesc.Format);

            if (uploadBufferSize != myCalculationsSize)
            {
                CONSOLE_ERROR_MESSAGE("INCORRECT UPLOAD BUFFER SIZE CALC // TRY TO USE MY CALCULATIONS! ");
                uploadBufferSize = myCalculationsSize;
            }
        }
        D3D12_SUBRESOURCE_DATA textureData = {};
        textureData.pData = pData;
        textureData.RowPitch = static_cast<LONG_PTR>(uploadBufferSize / pResource.Get()->GetDesc().Height);
        textureData.SlicePitch = uploadBufferSize;

        if (!upBuffer)
            upBuffer.reset(new UploadBuffer<char>(pDevice, (UINT)uploadBufferSize, false));

        ResourceBarrierChange(pCommandList, 1, D3D12_RESOURCE_STATE_COPY_SOURCE);

        UpdateSubresources(pCommandList,
            pResource.Get(),
            upBuffer->GetResource(),
            0, 0, 1, &textureData);

        ResourceBarrierChange(pCommandList, 1, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }

    bool FResource::DeleteUploadBuffer()
    {
        upBuffer.reset();
        return !upBuffer;
    }
    
    void FResource::ResourceBarrierChange(ID3D12GraphicsCommandList* pCommandList, const UINT numBariers, const D3D12_RESOURCE_STATES resourceStateAfter)
    {
        if (currState != resourceStateAfter)
        {
            pCommandList->ResourceBarrier(1,
                &keep(CD3DX12_RESOURCE_BARRIER::Transition(pResource.Get(),
                    currState,
                    resourceStateAfter)));

            currState = resourceStateAfter;
        }
    }

    ID3D12Resource* FResource::GetResource() const
    {
        return pResource.Get();
    }

    void FResource::ReleaseUploadBuffers() 
    {
        for (auto& el : FResource::textures)
        {
            auto ptr = el.second.lock();
            if (ptr && ptr->upBuffer) 
            {
                auto h = ptr->upBuffer.release();
                delete h;
            }
        }
    }

}