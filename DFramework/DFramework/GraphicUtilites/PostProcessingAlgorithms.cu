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


__global__ void InvertColorsKernel(cudaSurfaceObject_t surface, unsigned int width, unsigned int height) {
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

__global__ void GreyColorsKernel(cudaSurfaceObject_t surface, unsigned int width, unsigned int height) {
	int x = blockIdx.x * blockDim.x + threadIdx.x;
	int y = blockIdx.y * blockDim.y + threadIdx.y;

	if (x >= width || y >= height) return;
	float4 pixel;

	surf2Dread(&pixel, surface, x * sizeof(float4), y);

	float average = (pixel.x + pixel.y + pixel.z) / 3.0;
	pixel = make_float4(average, average, average, pixel.w);

	surf2Dwrite(pixel, surface, x * sizeof(float4), y);
}

__global__ void SharpnessColorsKernel(cudaSurfaceObject_t surface, unsigned int width, unsigned int height) {
	int x = blockIdx.x * blockDim.x + threadIdx.x;
	int y = blockIdx.y * blockDim.y + threadIdx.y;

	if (x >= width-1 || y >= height-1 || x < 1 || y < 1) return;
	

	int kernel[3][3] = { { 0, -1,  0 },
						 { -1,  5, -1 },
						 { 0, -1,  0 } };

	float4 pixel, outputPixel = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
	int2 texSize = make_int2(width, height);

	for (int ky = -1; ky <= 1; ky++) {
		for (int kx = -1; kx <= 1; kx++) {
			int2 neighborPos = make_int2(min(max(x + kx, 0), texSize.x - 1),
				min(max(y + ky, 0), texSize.y - 1));

			surf2Dread(&pixel, surface, neighborPos.x * sizeof(float4), neighborPos.y);

			outputPixel.x += kernel[ky + 1][kx + 1] * pixel.x;
			outputPixel.y += kernel[ky + 1][kx + 1] * pixel.y;
			outputPixel.z += kernel[ky + 1][kx + 1] * pixel.z;
		}
	}

	outputPixel.x = min(max(outputPixel.x, 0.0f), 1.0f);
	outputPixel.y = min(max(outputPixel.y, 0.0f), 1.0f);
	outputPixel.z = min(max(outputPixel.z, 0.0f), 1.0f);
	outputPixel.w = 1.0f;

	surf2Dwrite(outputPixel, surface, x * sizeof(float4), y);
}

__global__ void BlurColorKernel(cudaSurfaceObject_t surface, unsigned int width, unsigned int height) {
	int x = blockIdx.x * blockDim.x + threadIdx.x;
	int y = blockIdx.y * blockDim.y + threadIdx.y;

	if (x >= width - 1 || y >= height - 1 || x < 1 || y < 1) return;

	float kernel[3][3] = { { 1.0f / 16, 2.0f / 16, 1.0f / 16 },
						   { 2.0f / 16, 4.0f / 16, 2.0f / 16 },
						   { 1.0f / 16, 2.0f / 16, 1.0f / 16 } };

	float4 pixel, outputPixel = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
	int2 texSize = make_int2(width, height);

	for (int ky = -1; ky <= 1; ky++) {
		for (int kx = -1; kx <= 1; kx++) {
			int2 neighborPos = make_int2(min(max(x + kx, 0), texSize.x - 1),
				min(max(y + ky, 0), texSize.y - 1));

			surf2Dread(&pixel, surface, neighborPos.x * sizeof(float4), neighborPos.y);

			outputPixel.x += kernel[ky + 1][kx + 1] * pixel.x;
			outputPixel.y += kernel[ky + 1][kx + 1] * pixel.y;
			outputPixel.z += kernel[ky + 1][kx + 1] * pixel.z;
		}
	}

	outputPixel.x = min(max(outputPixel.x, 0.0f), 1.0f);
	outputPixel.y = min(max(outputPixel.y, 0.0f), 1.0f);
	outputPixel.z = min(max(outputPixel.z, 0.0f), 1.0f);
	outputPixel.w = 1.0f;

	surf2Dwrite(outputPixel, surface, x * sizeof(float4), y);
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
	unsigned width = (unsigned)texDesc.Width;
	unsigned height = (unsigned)texDesc.Height;

	dim3 blockSize(16, 16);
	dim3 gridSize((width + blockSize.x - 1) / blockSize.x,
		(height + blockSize.y - 1) / blockSize.y);
	GreyColorsKernel <<<gridSize, blockSize>>>(surfacesMap[texture].surface, width, height);

	cudaError_t err = cudaDeviceSynchronize();
}

void GreyEffect(ID3D12Resource* texture, ID3D12Device* device) 
{
	if (!FindInMap(texture)) {
		PRINTF_FDW("CANT FIND TEXTURE SURFACE /// CALL INIT TO MAP");
		InitToMap(texture, device);
	}

	D3D12_RESOURCE_DESC texDesc = texture->GetDesc();
	unsigned width = (unsigned)texDesc.Width;
	unsigned height = (unsigned)texDesc.Height;

	dim3 blockSize(16, 16);
	dim3 gridSize((width + blockSize.x - 1) / blockSize.x,
		(height + blockSize.y - 1) / blockSize.y);
	GreyColorsKernel <<<gridSize, blockSize>>>(surfacesMap[texture].surface, width, height);

	cudaError_t err = cudaDeviceSynchronize();
}

void SharpnessEffect(ID3D12Resource* texture, ID3D12Device* device)
{
	if (!FindInMap(texture)) {
		PRINTF_FDW("CANT FIND TEXTURE SURFACE /// CALL INIT TO MAP");
		InitToMap(texture, device);
	}

	D3D12_RESOURCE_DESC texDesc = texture->GetDesc();
	unsigned width = (unsigned)texDesc.Width;
	unsigned height = (unsigned)texDesc.Height;

	dim3 blockSize(16, 16);
	dim3 gridSize((width + blockSize.x - 1) / blockSize.x,
		(height + blockSize.y - 1) / blockSize.y);
	SharpnessColorsKernel <<<gridSize, blockSize>>>(surfacesMap[texture].surface, width, height);

	cudaError_t err = cudaDeviceSynchronize();
}

void BlurEffect(ID3D12Resource* texture, ID3D12Device* device)
{
	if (!FindInMap(texture)) {
		PRINTF_FDW("CANT FIND TEXTURE SURFACE /// CALL INIT TO MAP");
		InitToMap(texture, device);
	}

	D3D12_RESOURCE_DESC texDesc = texture->GetDesc();
	unsigned width = (unsigned)texDesc.Width;
	unsigned height = (unsigned)texDesc.Height;

	dim3 blockSize(16, 16);
	dim3 gridSize((width + blockSize.x - 1) / blockSize.x,
		(height + blockSize.y - 1) / blockSize.y);
	BlurColorKernel <<<gridSize, blockSize>>>(surfacesMap[texture].surface, width, height);

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
