#include "TextureLoaders.h"

#define DDS_MAGIC 0x20534444 
#define DDS_FOURCC 0x00000004
#define DDS_RGB    0x00000040
#define DDS_RGBA   0x00000041
#define DDS_HEADER_FLAGS_TEXTURE 0x00001007
#define DDS_HEIGHT 0x00000002
#define DDS_WIDTH  0x00000004
#define DDS_PIXELFORMAT_DX10 0x30315844 
#define DDSCAPS2_CUBEMAP 0x00000200
#define DDSCAPS2_CUBEMAP_ALLFACES 0x0000FC00

namespace FD3DW {




struct DDS_PIXELFORMAT
{
    uint32_t size;
    uint32_t flags;
    uint32_t fourCC;
    uint32_t RGBBitCount;
    uint32_t RBitMask;
    uint32_t GBitMask;
    uint32_t BBitMask;
    uint32_t ABitMask;
};

struct DDS_HEADER
{
    uint32_t size;
    uint32_t flags;
    uint32_t height;
    uint32_t width;
    uint32_t pitchOrLinearSize;
    uint32_t depth;
    uint32_t mipMapCount;
    uint32_t reserved1[11];
    DDS_PIXELFORMAT ddspf;
    uint32_t caps;
    uint32_t caps2;
    uint32_t caps3;
    uint32_t caps4;
    uint32_t reserved2;
};

struct DDS_HEADER_DXT10
{
    DXGI_FORMAT dxgiFormat;
    D3D12_RESOURCE_DIMENSION resourceDimension;
    uint32_t miscFlag;
    uint32_t arraySize;
    uint32_t miscFlags2;
};

static size_t BitsPerPixel(DXGI_FORMAT fmt)
{
    switch (fmt)
    {
    case DXGI_FORMAT_R32G32B32A32_FLOAT: return 128;
    case DXGI_FORMAT_R16G16B16A16_FLOAT: return 64;
    case DXGI_FORMAT_R8G8B8A8_UNORM: return 32;
    case DXGI_FORMAT_BC1_UNORM: return 4;
    case DXGI_FORMAT_BC2_UNORM: return 8;
    case DXGI_FORMAT_BC3_UNORM: return 8;
    case DXGI_FORMAT_BC4_UNORM: return 4;
    case DXGI_FORMAT_BC5_UNORM: return 8;
    case DXGI_FORMAT_BC6H_UF16: return 8;
    case DXGI_FORMAT_BC7_UNORM: return 8;
    default: return 0;
    }
}

static DXGI_FORMAT GetDXGIFormat(const DDS_PIXELFORMAT& ddpf)
{
    if (ddpf.flags & DDS_FOURCC)
    {
        switch (ddpf.fourCC)
        {
        case '1TXD': return DXGI_FORMAT_BC1_UNORM;
        case '3TXD': return DXGI_FORMAT_BC2_UNORM;
        case '5TXD': return DXGI_FORMAT_BC3_UNORM;
        case 'U4CB': return DXGI_FORMAT_BC4_UNORM;
        case 'U5CB': return DXGI_FORMAT_BC5_UNORM;
        case DDS_PIXELFORMAT_DX10: return DXGI_FORMAT_UNKNOWN;
        default: return DXGI_FORMAT_UNKNOWN;
        }
    }
    else if (ddpf.flags & DDS_RGBA && ddpf.RGBBitCount == 32)
    {
        return DXGI_FORMAT_R8G8B8A8_UNORM;
    }
    return DXGI_FORMAT_UNKNOWN;
}

static HRESULT LoadDDSData(const std::string& path, std::vector<uint8_t>& ddsData, DDS_HEADER& header, DDS_HEADER_DXT10* dx10HeaderOut)
{
    std::ifstream file(path, std::ios::binary);
    if (!file) return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

    uint32_t magic = 0;
    file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    if (magic != DDS_MAGIC) return E_FAIL;

    file.read(reinterpret_cast<char*>(&header), sizeof(DDS_HEADER));
    if (header.size != 124 || header.ddspf.size != 32) return E_FAIL;

    bool hasDX10 = (header.ddspf.flags & DDS_FOURCC) && (header.ddspf.fourCC == DDS_PIXELFORMAT_DX10);
    DDS_HEADER_DXT10 dx10Header = {};

    if (hasDX10)
    {
        file.read(reinterpret_cast<char*>(&dx10Header), sizeof(DDS_HEADER_DXT10));
        if (dx10HeaderOut) *dx10HeaderOut = dx10Header;
    }

    file.seekg(0, std::ios::end);
    size_t size = static_cast<size_t>(file.tellg());
    size_t dataOffset = sizeof(magic) + sizeof(DDS_HEADER) + (hasDX10 ? sizeof(DDS_HEADER_DXT10) : 0);
    size_t dataSize = size - dataOffset;
    ddsData.resize(dataSize);
    file.seekg(dataOffset, std::ios::beg);
    file.read(reinterpret_cast<char*>(ddsData.data()), dataSize);

    return S_OK;
}

HRESULT DDSTextureLoaderDX12::Load(
    const std::string& path,
    ID3D12Device* device,
    ID3D12GraphicsCommandList* commandList,
    TextureDataOutput& outTexture)
{
    DDS_HEADER header = {};
    DDS_HEADER_DXT10 dx10Header = {};
    std::vector<uint8_t> ddsData;

    HRESULT hr = LoadDDSData(path, ddsData, header, &dx10Header);
    if (FAILED(hr)) return hr;

    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
    D3D12_RESOURCE_DIMENSION dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    UINT arraySize = 1;
    UINT mipCount = header.mipMapCount ? header.mipMapCount : 1;

    if ((header.ddspf.flags & DDS_FOURCC) && header.ddspf.fourCC == DDS_PIXELFORMAT_DX10)
    {
        format = dx10Header.dxgiFormat;
        dimension = dx10Header.resourceDimension;
        arraySize = dx10Header.arraySize;
    }
    else
    {
        format = GetDXGIFormat(header.ddspf);
        arraySize = 1;

        if ((header.caps2 & DDSCAPS2_CUBEMAP) && (header.caps2 & DDSCAPS2_CUBEMAP_ALLFACES) == DDSCAPS2_CUBEMAP_ALLFACES)
        {
            dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            arraySize = 6;
        }
        else
        {
            dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        }
    }

    if (format == DXGI_FORMAT_UNKNOWN)
        return E_FAIL;

    UINT width = header.width;
    UINT height = header.height;
    UINT depth = header.depth ? header.depth : 1;

    CD3DX12_RESOURCE_DESC textureDesc = {};
    switch (dimension)
    {
    case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
        textureDesc = CD3DX12_RESOURCE_DESC::Tex1D(format, width, arraySize, mipCount);
        break;
    case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
        textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(format, width, height, arraySize, mipCount);
        break;
    case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
        textureDesc = CD3DX12_RESOURCE_DESC::Tex3D(format, width, height, depth, mipCount);
        break;
    default:
        return E_FAIL;
    }

    hr = device->CreateCommittedResource(
        &FD3DW::keep(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)),
        D3D12_HEAP_FLAG_NONE,
        &textureDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&outTexture.TextureResource));
    if (FAILED(hr)) return hr;

    UINT64 uploadBufferSize = GetRequiredIntermediateSize(outTexture.TextureResource.Get(), 0, mipCount * arraySize);

    outTexture.ResultUploadBuffer = std::make_unique<UploadBuffer<char>>(device, static_cast<UINT>(uploadBufferSize), false);

    std::vector<D3D12_SUBRESOURCE_DATA> subresources;
    size_t offset = 0;

    for (UINT arrayIdx = 0; arrayIdx < arraySize; ++arrayIdx)
    {
        UINT w = width;
        UINT h = height;
        UINT d = depth;
        for (UINT mip = 0; mip < mipCount; ++mip)
        {
            bool isBC = format == DXGI_FORMAT_BC1_UNORM || format == DXGI_FORMAT_BC2_UNORM ||
                format == DXGI_FORMAT_BC3_UNORM || format == DXGI_FORMAT_BC4_UNORM ||
                format == DXGI_FORMAT_BC5_UNORM || format == DXGI_FORMAT_BC6H_UF16 ||
                format == DXGI_FORMAT_BC7_UNORM;

            size_t rowPitch = 0;
            size_t slicePitch = 0;

            if (isBC)
            {
                UINT blockSize = (BitsPerPixel(format) == 4) ? 8 : 16;
                UINT numBlocksWide = std::max(1u, (w + 3) / 4);
                UINT numBlocksHigh = std::max(1u, (h + 3) / 4);
                rowPitch = numBlocksWide * blockSize;
                slicePitch = rowPitch * numBlocksHigh;
            }
            else
            {
                rowPitch = (w * BitsPerPixel(format) + 7) / 8;
                slicePitch = rowPitch * h;
            }

            D3D12_SUBRESOURCE_DATA sub = {};
            sub.pData = ddsData.data() + offset;
            sub.RowPitch = rowPitch;
            sub.SlicePitch = slicePitch;

            subresources.push_back(sub);
            offset += slicePitch;

            w = std::max(w >> 1, 1u);
            h = std::max(h >> 1, 1u);
        }
    }

    UpdateSubresources(commandList, outTexture.TextureResource.Get(), outTexture.ResultUploadBuffer->GetResource(), 0, 0, (UINT)subresources.size(), subresources.data());

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        outTexture.TextureResource.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    commandList->ResourceBarrier(1, &barrier);

    return S_OK;
}



}