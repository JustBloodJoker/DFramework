#pragma once

#include <D3DFramework/GraphicUtilites/RenderThreadUtils/ExecutionHandle.h>

#include <ShadowData/CaptureConfiguration.h>
#include <RayTracing/GroundTruth/NeuralShadowRTTargetProducer.h>
#include <Capture/NeuralShadowInputCapture.h>

class OfflineRenderer;

struct NeuralShadowCaptureRunOptions {
    FDW_SD::Capture::NSCaptureConfig Config{};
    std::filesystem::path DatasetRoot;
    bool Resume = false;
};

struct NeuralShadowCaptureFrameContext {
    std::vector<std::shared_ptr<FD3DW::ExecutionHandle>> Dependencies;
};

class NeuralShadowDatasetWriter {
public:
    NSNeuralShadowDataset::NSDatasetResult Publish(const NeuralShadowCaptureRunOptions& options, const FDW_SD::Capture::NSCaptureSamplePlan& sample, std::span<const FDW_SD::Capture::NSCaptureOwnedBlock> blocks) const;

    std::uint64_t PublishedBytes = 0u;
};

class NeuralShadowCaptureController {
public:
    explicit NeuralShadowCaptureController(NeuralShadowCaptureRunOptions options);

public:
    std::uint64_t GetGenerationOverride() const;
    bool Initialize(OfflineRenderer& renderer);
    bool ConfigureSample(OfflineRenderer& renderer);
   
    std::shared_ptr<FD3DW::ExecutionHandle> Schedule(OfflineRenderer& renderer, const std::vector<std::shared_ptr<FD3DW::ExecutionHandle>>& dependencies);
    bool Finalize(OfflineRenderer& renderer, const std::shared_ptr<FD3DW::ExecutionHandle>& captureHandle);
    void Finish(OfflineRenderer& renderer, bool succeeded, std::string message = {});
    
public:
    int GetExitCode() const;
    bool IsConfigured() const;
    bool IsFinished() const;
    bool IsWarmupFrame() const;
    const NeuralShadowCaptureRunOptions& GetOptions() const;

protected:
	static std::vector<std::byte> CopyBytes(const void* data, std::size_t byteCount);
	
    static bool BuildNeuralOutputBlocks(
		const FDW_SD::Capture::NSCaptureSamplePlan& sample,
		const FDW_SD::Work::NSWorkDispatchHeader& sourceHeader,
		std::span<const std::byte> sourceWorkBytes,
		std::span<const std::byte> sourcePixelBytes,
		std::span<const std::byte> sourceLightBytes,
		std::span<const std::byte> sourceHardBytes,
		std::span<const std::byte> sourceTargetBytes,
		std::span<const std::byte> sourceTargetValidityBytes,
		std::span<const std::byte> sourceBlockerBytes,
		std::span<const std::byte> sourceInputValidityBytes,
		std::vector<FDW_SD::Capture::NSCaptureOwnedBlock>& blocks,
		std::string& error);


    void WriteReport(bool succeeded, const std::string& message) const;
    
    void AppendJournal(
        const FDW_SD::Capture::NSCaptureSamplePlan& sample,
        std::string_view status,
        double elapsedSeconds,
        std::uint64_t byteSize,
        std::string_view message = {}) const;

public:
    std::optional<NeuralShadowCaptureRunOptions> m_xCaptureOptions;
    std::vector<FDW_SD::Capture::NSCaptureSamplePlan> m_vCapturePlan;

    std::size_t m_uCaptureSampleIndex = 0u;
    std::uint32_t m_uCaptureGenerated = 0u;
    std::uint32_t m_uCaptureSkipped = 0u;
    std::uint32_t m_uCaptureWarmupFramesRemaining = 0u;
    std::uint64_t m_uCaptureOutputBytes = 0u;
    
    std::chrono::steady_clock::time_point m_xCaptureStartTime{};
    std::chrono::steady_clock::time_point m_xCaptureSampleStartTime{};
    
    std::unique_ptr<FDW_SD::GroundTruth::NeuralShadowRTTargetProducer> m_pCaptureRTTargetProducer;
    std::unique_ptr<NeuralShadowInputCapture> m_pInputCapture;
    std::atomic<bool> m_bCaptureRecordSucceeded{ false };
    
    int m_iCaptureExitCode = 0;
    bool m_bCaptureFinished = false;
    bool m_bCaptureSampleActive = false;
    
    std::string m_sCaptureLastError;
    NeuralShadowDatasetWriter DatasetWriter;
};
