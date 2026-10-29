#pragma once

#include <Renderer/OfflineRendererComponent.h>
#include <Component/Shadow/ShadowComponentData.h>
#include <System/Shadows/ShadowRenderServices.h>
#include <System/Shadows/Atlas/ShadowAtlasRenderer.h>
#include <System/Shadows/Neural/NeuralShadowWorkGenerator.h>
#include <System/Shadows/Neural/NeuralShadowBaselinePass.h>

#include <atomic>
#include <mutex>
#include <vector>

class ShadowSystem final : public OfflineRendererComponent, public IShadowRenderServices {
public:
	void AfterConstruction() override;
	void ProcessNotify(NRenderSystemNotifyType type) override;
	
public:
	std::vector<ShadowComponentData> GetUserSettingsSnapshot() const;
	
	void EnsureCaptureResources();
	
public:
	std::shared_ptr<FD3DW::ExecutionHandle> ScheduleMetadata(const std::vector<std::shared_ptr<FD3DW::ExecutionHandle>>& dependencies);
	std::shared_ptr<FD3DW::ExecutionHandle> ScheduleNeuralWork(const std::vector<std::shared_ptr<FD3DW::ExecutionHandle>>& dependencies, std::uint64_t generationOverride);
	std::shared_ptr<FD3DW::ExecutionHandle> ScheduleHardVisibility( const std::vector<std::shared_ptr<FD3DW::ExecutionHandle>>& dependencies);
	std::shared_ptr<FD3DW::ExecutionHandle> ScheduleNeuralBaseline( const std::vector<std::shared_ptr<FD3DW::ExecutionHandle>>& dependencies);
	
public:
	void ConfigureInputResources(FD3DW::FResource* normal, ID3D12Device* device);
	
	FD3DW::FResource* EnsureBlockerDistanceAtlasResource();
	FD3DW::FResource* GetBlockerValidityAtlasResource() const;
	
	void SetHardFilterSettings(const HardShadowAtlasFilterSettings& settings);
	void InitializeBackend();
	
	NeuralShadowWorkGenerator* GetNeuralWorkGenerator() const noexcept;
	NeuralShadowBaselinePass* GetNeuralBaselinePass() const noexcept;

public:
	ID3D12Device* GetDevice() const override;
	ID3D12Device5* GetDXRDevice() const override;
	
	int GetSceneRenderWidth() const override;
	int GetSceneRenderHeight() const override;

	DirectX::XMFLOAT3 GetCurrentCameraPosition() const override;
	DirectX::XMMATRIX GetViewProjectionMatrix() const override;
	
	const std::vector<LightComponentData>& GetLightComponentsData() const override;
	std::vector<ShadowComponentData> GetShadowLightSettings() const override;
	
	ID3D12Resource* GetTLASResource() const override;
	
	FD3DW::DepthStencilView* GetCurrentDepthResource() const override;
	
	D3D12_GPU_VIRTUAL_ADDRESS GetLightsBufferAddress() const override;
	
	NSRTGeometryCoverageState GetRTGeometryCoverage() const override;
	
	int GetLightsCount() const override;
	UINT GetShadowMetadataCount() const override;
	
	void PrepareShadowMetadataForCompute(ID3D12GraphicsCommandList* list) const override;
	void PrepareShadowMetadataForPixel(ID3D12GraphicsCommandList* list) const override;
	
	D3D12_GPU_VIRTUAL_ADDRESS GetClusterBufferAddress() const override;
	D3D12_GPU_VIRTUAL_ADDRESS GetShadowMetadataAddress() const override;
	
	FD3DW::FResource* GetShadowMetadataResource() const;
	
	float GetCameraNear() const override;
	float GetCameraFar() const override;
	
	FD3DW::StructuredBuffer* GetLightsBuffer() const override;
	
	D3D12_GPU_VIRTUAL_ADDRESS GetNeuralWorkItemsAddress() const override;
	D3D12_GPU_VIRTUAL_ADDRESS GetNeuralWorkHeaderAddress() const override;

protected:
	void EnsureNeuralWorkGenerator();
	void EnsureNeuralBaseline();
	void RefreshUserSettings();

protected:
	mutable std::mutex m_xSettingsMutex;
	
	std::vector<ShadowComponentData> m_vUserSettings;
	std::atomic<bool> m_bNeedsRefresh{ true };
	std::unique_ptr<ShadowAtlasRenderer> m_pAtlasBackend;
	std::unique_ptr<NeuralShadowWorkGenerator> m_pNeuralWorkGenerator;
	std::unique_ptr<NeuralShadowBaselinePass> m_pNeuralBaselinePass;
	
	FD3DW::FResource* m_pNormalInput = nullptr;
};
