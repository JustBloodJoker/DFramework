#include "PostProcessingAlgorithms.cuh"

#include "cuda.h"
#include "cuda_runtime.h"
#include "device_launch_parameters.h"

void CheckCudaErrors(cudaError_t error) {
	printf("\nCUDA ERROR --- %d (NULL IT'S OK) \n", error);
}

struct CUDASURFACEDATA {

	cudaSurfaceObject_t surface;
	cudaMipmappedArray_t cuMipArray;
	cudaExternalMemory_t cudaExternalMemory;

	void CLEAR() {
		cudaFreeMipmappedArray(cuMipArray);
		cudaDestroyExternalMemory(cudaExternalMemory);
		cudaDestroySurfaceObject(surface);
	}
};

std::unordered_map<ID3D12Resource*, CUDASURFACEDATA> surfacesMap;


__global__ void invertColorsKernel(cudaSurfaceObject_t surface, unsigned int width, unsigned int height) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

	if (x >= width || y >= height) return;
	float4 pixel;

	surf2Dread(&pixel, surface, x * sizeof(float4), y);

	pixel.x = 1.0f - pixel.x;
	pixel.y = 1.0f - pixel.y;
	pixel.z = 1.0f - pixel.z;

	surf2Dwrite(pixel, surface, x * sizeof(float4), y );
}


CUDASURFACEDATA CreateSurfaceObject(ID3D12Resource* texture, ID3D12Device* device) {
	CUDASURFACEDATA data{};

	D3D12_RESOURCE_DESC texDesc = texture->GetDesc();
	HANDLE sharedHandle{};
	SECURITY_ATTRIBUTES secAttr{};
	LPCWSTR name{};
	hr = device->CreateSharedHandle(texture, &secAttr, GENERIC_ALL, name, &sharedHandle);
	const auto texAllocInfo = device->GetResourceAllocationInfo(NULL, 1, &texDesc);

	cudaExternalMemoryHandleDesc cuExtmemHandleDesc{};
	cuExtmemHandleDesc.type = cudaExternalMemoryHandleTypeD3D12Heap;
	cuExtmemHandleDesc.handle.win32.handle = sharedHandle;
	cuExtmemHandleDesc.size = texAllocInfo.SizeInBytes;
	cuExtmemHandleDesc.flags = cudaExternalMemoryDedicated;
	CheckCudaErrors(cudaImportExternalMemory(&data.cudaExternalMemory, &cuExtmemHandleDesc));

	cudaExternalMemoryMipmappedArrayDesc cuExtmemMipDesc{};
	cuExtmemMipDesc.extent = make_cudaExtent(texDesc.Width, texDesc.Height, texDesc.DepthOrArraySize);
	cuExtmemMipDesc.formatDesc = cudaCreateChannelDesc(32, 32, 32, 32, cudaChannelFormatKindFloat);
	cuExtmemMipDesc.numLevels = 1;

	CheckCudaErrors(cudaExternalMemoryGetMappedMipmappedArray(&data.cuMipArray, data.cudaExternalMemory, &cuExtmemMipDesc));

	cudaArray_t cuArray{};
	CheckCudaErrors(cudaGetMipmappedArrayLevel(&cuArray, data.cuMipArray, 0));

	cudaSurfaceObject_t cuSurface{};
	cudaResourceDesc cuResDesc{};
	cuResDesc.resType = cudaResourceTypeArray;
	cuResDesc.res.array.array = cuArray;
	CheckCudaErrors(cudaCreateSurfaceObject(&data.surface, &cuResDesc));

	return data;
}

void InverseTexture(ID3D12Resource* texture, ID3D12Device* device)
{
	if (!FindInMap(texture)) {
		PRINTF_FDW("CANT FIND TEXTURE SURFACE /// CALL INIT TO MAP");
		InitToMap(texture, device);
	}

	D3D12_RESOURCE_DESC texDesc = texture->GetDesc();
	dim3 blockSize(16, 16);
	dim3 gridSize((texDesc.Width + blockSize.x - 1) / blockSize.x,
		(texDesc.Height + blockSize.y - 1) / blockSize.y);
	invertColorsKernel<<<gridSize, blockSize>>>(surfacesMap[texture].surface, texDesc.Width, texDesc.Height);

	cudaError_t err = cudaDeviceSynchronize();
}

void ClearFromMap(ID3D12Resource* texture) {
	surfacesMap[texture].CLEAR();
	surfacesMap.erase(texture);
}

void ClearAllFromMap() {
	for (auto& el : surfacesMap) {
		el.second.CLEAR();
	}
	surfacesMap.clear(); 
}

void InitToMap(ID3D12Resource* texture, ID3D12Device* device) {
	auto iter = surfacesMap.find(texture);
	if (iter!=surfacesMap.end()) {
		PRINTF_FDW("TEXTURE ALREADY IN SURFACE MAP");
		return;
	}

	auto data = CreateSurfaceObject(texture, device);
	if (data.surface) surfacesMap[texture] = data;
}

bool FindInMap(ID3D12Resource* texture) {
	return surfacesMap.find(texture)!=surfacesMap.end();
}
