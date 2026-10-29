#pragma once

#include <d3d12.h>
#include <dxgiformat.h>

struct GBuffersData {
	std::vector<DXGI_FORMAT> GBuffersFormats;
};

const GBuffersData& GetGBufferData();
UINT GetGBuffersNum();

void InitializeDescriptorSizes(ID3D12Device* device, UINT rtvDescSize, UINT cbvSrvUavDescSize);
const UINT& GetRTVDescriptorSize(ID3D12Device* device);
const UINT& GetCBV_SRV_UAVDescriptorSize(ID3D12Device* device);
