#pragma once

#include <MainRenderer/MainRendererComponent.h>
#include <Component/Shadow/ShadowComponentData.h>
#include <System/Shadows/ShadowRouter.h>
#include <System/Shadows/ShadowRenderServices.h>
#include <System/Shadows/Atlas/ShadowAtlasRenderer.h>
#include <System/Shadows/Neural/NeuralShadowWorkGenerator.h>
#include <System/Shadows/Neural/NeuralShadowBaselinePass.h>
#include <System/Shadows/Neural/NeuralShadowInferencePass.h>

#include <atomic>
#include <mutex>
#include <string>
#include <vector>

enum class NeuralShadowRuntimeStatus : std::uint32_t {
	Ready = 0,
	Loading = 1,
	Unavailable = 2,
	Error = 3
};

struct NeuralShadowRuntimeState {
	NeuralShadowRuntimeStatus Status = NeuralShadowRuntimeStatus::Unavailable;
	bool Selected = false;
	bool Loaded = false;
	std::string Reason;
};

class ShadowSystem final : public MainRendererComponent, public IShadowRenderServices {

public:
	void AfterConstruction() override;
	void ProcessNotify(NRenderSystemNotifyType type) override;

public:
	std::vector<ShadowComponentData> GetUserSettingsSnapshot() const;

public:
	ShadowFrameContext BuildFrameContext(std::uint64_t frameIndex,ShadowMode requestedMode,ShadowBackendAvailability availability,std::uint32_t lightCount) const;
	ShadowVisibilityView BuildVisibilityView(const ShadowFrameContext& context, D3D12_GPU_VIRTUAL_ADDRESS emptyBuffer, D3D12_GPU_VIRTUAL_ADDRESS neutralParameters) const;
	ShadowBackendState GetState() const;
	ShadowBackendAvailability GetAvailability(bool unlit, bool rtSupported) const;

public:
	void EnsureProducerResources(const ShadowProducerRoutingState& producers);

public:
	std::shared_ptr<FD3DW::ExecutionHandle> ScheduleMetadata(const std::vector<std::shared_ptr<FD3DW::ExecutionHandle>>& dependencies);

	std::shared_ptr<FD3DW::ExecutionHandle> ScheduleNeuralWork(const std::vector<std::shared_ptr<FD3DW::ExecutionHandle>>& dependencies, bool spatial);

	std::shared_ptr<FD3DW::ExecutionHandle> ScheduleHardVisibility(const std::vector<std::shared_ptr<FD3DW::ExecutionHandle>>& dependencies);

	std::shared_ptr<FD3DW::ExecutionHandle> ScheduleRayTracedVisibility(const std::vector<std::shared_ptr<FD3DW::ExecutionHandle>>& dependencies);

	std::shared_ptr<FD3DW::ExecutionHandle> ScheduleNeuralOutput(const std::vector<std::shared_ptr<FD3DW::ExecutionHandle>>& dependencies, bool useInference);

public:
	void ConfigureInputResources(FD3DW::FResource* normal, ID3D12Device* device);

public:
	FD3DW::FResource* GetShadowAtlasResource() const;
	FD3DW::FResource* EnsureBlockerDistanceAtlasResource();
	FD3DW::FResource* GetBlockerValidityAtlasResource() const;

public:
	NSRTQualitySettings GetRTQualitySettings() const;
	void SetRTQualitySettings(const NSRTQualitySettings& settings);
	HardShadowAtlasFilterSettings GetHardFilterSettings() const;
	void SetHardFilterSettings(const HardShadowAtlasFilterSettings& settings);

public:
	void InitializeBackend();

public:
	NeuralShadowInferencePass::State EnsureNeuralRuntimeInitialized();
	bool SelectNeuralRuntimeAfterGpuIdle();
	bool IsNeuralRuntimeSelected() const noexcept;
	NeuralShadowRuntimeState GetNeuralRuntimeState(bool rtSupported) const;
	bool IsNeuralInferenceReady() const;

public:
	ID3D12Device* GetDevice() const override;
	ID3D12Device5* GetDXRDevice() const override;

	int GetSceneRenderWidth() const override;
	int GetSceneRenderHeight() const override;

	DirectX::XMFLOAT3 GetCurrentCameraPosition() const override;
	float GetCameraZFar() const override;
	DirectX::XMMATRIX GetJitteredViewProjectionMatrix() const override;

	const std::vector<LightComponentData>& GetLightComponentsData() const override;
	const std::vector<std::uint64_t>& GetLightRuntimeIdentities() const override;

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

	FD3DW::FResource* GetNeuralWorkItemsResource() const override;
	FD3DW::FResource* GetNeuralWorkHeaderResource() const override;

protected:

	void EnsureNeuralWorkGenerator();
	void EnsureNeuralBaseline();
	void RefreshUserSettings();

protected:
	std::filesystem::path SelectedNeuralModel() const;

protected:
	mutable std::mutex m_xSettingsMutex;
	std::vector<ShadowComponentData> m_vUserSettings;
	std::atomic<bool> m_bNeedsRefresh{ true };

	ShadowRouter m_xRouter;
	mutable std::mutex m_xStateMutex;
	mutable ShadowBackendState m_xState;

	std::unique_ptr<ShadowAtlasRenderer> m_pAtlasBackend;
	std::unique_ptr<NeuralShadowWorkGenerator> m_pNeuralWorkGenerator;
	std::unique_ptr<NeuralShadowBaselinePass> m_pNeuralBaselinePass;
	std::unique_ptr<NeuralShadowInferencePass> m_pNeuralShadowInferencePass;

	bool m_bNeuralSelected = false;
	std::atomic<bool> m_bNeuralInitializationInProgress{ false };
	FD3DW::FResource* m_pNormalInput = nullptr;
};
