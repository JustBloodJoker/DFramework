#pragma once

#include <d3d12.h>

void CreateEmptyStructuredBuffer(ID3D12Device* device);
D3D12_GPU_VIRTUAL_ADDRESS GetEmptyStructuredBufferGPUVirtualAddress();
