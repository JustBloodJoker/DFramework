#pragma once

#include <d3d12.h>
#include <dxgiformat.h>

struct GBuffersData {
	std::vector<DXGI_FORMAT> GBuffersFormats;
};

const GBuffersData& GetGBufferData();
const DXGI_FORMAT GetForwardRenderPassFormat();
UINT GetGBuffersNum();

void InitializeDescriptorSizes(ID3D12Device* device, UINT rtvDescSize, UINT dsvDescSize, UINT cbvSrvUavDescSize);
const UINT& GetDSVDescriptorSize(ID3D12Device* device);
const UINT& GetRTVDescriptorSize(ID3D12Device* device);
const UINT& GetCBV_SRV_UAVDescriptorSize(ID3D12Device* device);
