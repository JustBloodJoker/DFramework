#pragma once

#include <Component/Light/LightComponentData.h>
#include <Component/Shadow/ShadowComponentData.h>
#include <System/Shadows/RayTracing/RTGeometryCoverage.h>

#include <d3d12.h>
#include <DirectXMath.h>

#include <vector>

namespace FD3DW { class DepthStencilView; }
namespace FD3DW { class StructuredBuffer; }

class IShadowRenderServices {
public:
	virtual ~IShadowRenderServices() = default;

	virtual ID3D12Device* GetDevice() const = 0;
	virtual ID3D12Device5* GetDXRDevice() const = 0;

	virtual int GetSceneRenderWidth() const = 0;
	virtual int GetSceneRenderHeight() const = 0;

	virtual DirectX::XMFLOAT3 GetCurrentCameraPosition() const = 0;
	virtual DirectX::XMMATRIX GetViewProjectionMatrix() const = 0;

	virtual const std::vector<LightComponentData>& GetLightComponentsData() const = 0;
	virtual std::vector<ShadowComponentData> GetShadowLightSettings() const = 0;
	
	virtual ID3D12Resource* GetTLASResource() const = 0;
	virtual FD3DW::DepthStencilView* GetCurrentDepthResource() const = 0;
	virtual D3D12_GPU_VIRTUAL_ADDRESS GetLightsBufferAddress() const = 0;
	virtual NSRTGeometryCoverageState GetRTGeometryCoverage() const = 0;
	
	virtual int GetLightsCount() const = 0;
	virtual UINT GetShadowMetadataCount() const = 0;
	
	virtual void PrepareShadowMetadataForCompute(ID3D12GraphicsCommandList* list) const = 0;
	virtual void PrepareShadowMetadataForPixel(ID3D12GraphicsCommandList* list) const = 0;
	
	virtual D3D12_GPU_VIRTUAL_ADDRESS GetClusterBufferAddress() const = 0;
	virtual D3D12_GPU_VIRTUAL_ADDRESS GetShadowMetadataAddress() const = 0;
	
	virtual float GetCameraNear() const = 0;
	virtual float GetCameraFar() const = 0;
	
	virtual FD3DW::StructuredBuffer* GetLightsBuffer() const = 0;
	
	virtual D3D12_GPU_VIRTUAL_ADDRESS GetNeuralWorkItemsAddress() const = 0;
	virtual D3D12_GPU_VIRTUAL_ADDRESS GetNeuralWorkHeaderAddress() const = 0;
};
