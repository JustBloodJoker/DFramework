#pragma once

#include <ShadowData/ShadowLightParameters.h>
#include <ShadowData/CaptureConfiguration.h>
#include <ShadowData/NeuralShadowData.h>
#include <System/Shadows/RayTracing/RTGeometryCoverage.h>

namespace FDW_SD::GroundTruth {

inline constexpr std::uint32_t RT_TARGET_METADATA_MAGIC = 0x3154'524eu; // NRT1
inline constexpr std::uint32_t RT_TARGET_METADATA_VERSION = 1u;
inline constexpr std::uint32_t RT_TARGET_METADATA_FORMAT_VERSION = 1u;
inline constexpr std::uint32_t RT_TARGET_COMPLETION_MARKER = 0x454e'4f44u; // DONE

struct NSRTTargetMetadataHeader {
	std::uint32_t Magic = RT_TARGET_METADATA_MAGIC;
	std::uint32_t Version = RT_TARGET_METADATA_VERSION;
	std::uint32_t HeaderByteSize = 0u;
	std::uint32_t FileByteSize = 0u;
	
	std::uint64_t SampleId = 0u;
	std::uint64_t GenerationId = 0u;
	std::uint64_t EngineRevisionHash = 0u;
	
	std::uint32_t EngineRevisionOffset = 0u;
	std::uint32_t EngineRevisionByteSize = 0u;
	std::uint32_t OutputWidth = 0u;
	std::uint32_t OutputHeight = 0u;
	std::uint32_t WorkItemCount = 0u;
	std::uint32_t OutputValueCount = 0u;
	std::uint32_t ValidValueCount = 0u;
	std::uint32_t InvalidValueCount = 0u;

	std::uint64_t CompletedRayCount = 0u;
	
	std::uint32_t OutputDomain = static_cast<std::uint32_t>(NSRTOutputDomain::ScreenResolutionNSOutput);
	std::uint32_t VisibilityEncoding = static_cast<std::uint32_t>(FDW_SD::Output::NSVisibilityEncoding::R16Float);
	std::uint32_t ValidityEncoding = static_cast<std::uint32_t>(FDW_SD::Output::NSValidityEncoding::R8Uint);
	std::uint32_t MetadataFormatVersion = RT_TARGET_METADATA_FORMAT_VERSION;
	
	NSRTQualityMetadata Quality{};
	
	NSRTGeometryCoverageState Geometry{};
	
	std::uint32_t Padding[11]{};
};

struct NSRTTargetCaptureEvidence {
	std::string EngineRevision;
	
	NSRTQualitySettings QualitySettings{
		NSRTQualityMode::OfflineGroundTruth,
		0u,
		NS_RT_OFFLINE_FINITE_SAMPLE_COUNT,
		NSRTDenoisingPolicy::Disabled,
		1u
	};

	NSRTGeometryCoverageState Geometry{};
};

struct NSRTTargetValidationStats {
	std::uint32_t ValidValueCount = 0u;
	std::uint32_t InvalidValueCount = 0u;
	
	std::uint64_t CompletedRayCount = 0u;
};

struct NSRTTargetDispatchAddress {
	std::uint32_t WorkItemIndex = 0u;
	std::uint32_t FirstLocalPixel = 0u;
};

NSRTTargetDispatchAddress ComputeRTTargetDispatchAddress(
	std::uint32_t workItemBase,
	std::uint32_t dispatchX,
	std::uint32_t dispatchY,
	std::uint32_t dispatchZ);

using NSRTGroundTruthProvider = FDW_SD::Capture::NSCaptureInputProvider;

class GroundTruthMetadata {
public:
	static FDW_SD::Capture::NSCaptureResult ValidateEvidence(const NSRTTargetCaptureEvidence& evidence);
	
	static FDW_SD::Capture::NSCaptureResult ValidateBlocks(const FDW_SD::Capture::NSCaptureSamplePlan& sample,std::span<const FDW_SD::Capture::NSCaptureOwnedBlock> blocks,const NSRTTargetCaptureEvidence& evidence,NSRTTargetValidationStats& stats);

public:
	static FDW_SD::Capture::NSCaptureResult Build(const FDW_SD::Capture::NSCaptureSamplePlan& sample,const NSRTTargetCaptureEvidence& evidence,const NSRTTargetValidationStats& stats, std::vector<std::byte>& bytes);
	
	static FDW_SD::Capture::NSCaptureResult Validate(std::span<const std::byte> bytes,const FDW_SD::Capture::NSCaptureSamplePlan* expectedSample = nullptr,
														const NSRTTargetCaptureEvidence* expectedEvidence = nullptr,NSRTTargetMetadataHeader* header = nullptr);

public:
	static FDW_SD::Capture::NSCaptureResult LoadPublished(const std::filesystem::path& datasetRoot,const std::filesystem::path& sampleRelativePath, const FDW_SD::Capture::NSCaptureSamplePlan* expectedSample,
															NSRTTargetMetadataHeader& header);
	
	static FDW_SD::Capture::NSCaptureResult ExecuteCapturePlan(const FDW_SD::Capture::NSCaptureConfig& config,const std::filesystem::path& datasetRoot,const NSRTTargetCaptureEvidence& evidence,
																const NSRTGroundTruthProvider& provider, FDW_SD::Capture::NSCaptureRunStats& stats);

protected:
	inline static constexpr std::uint64_t FNV_OFFSET = 14695981039346656037ull;
	inline static constexpr std::uint64_t FNV_PRIME = 1099511628211ull;

protected:
	static FDW_SD::Capture::NSCaptureResult Failure(NSNeuralShadowDataset::NSDatasetErrorCode code, std::string message);
	
	static std::uint64_t HashRevision(std::string_view revision);
	
	static const FDW_SD::Capture::NSCaptureOwnedBlock* FindOwnedBlock(std::span<const FDW_SD::Capture::NSCaptureOwnedBlock> blocks,NSNeuralShadowDataset::NSSampleBlockKind kind);
	
	static const NSNeuralShadowDataset::NSLoadedSampleBlock* FindLoadedBlock(const NSNeuralShadowDataset::NSLoadedSample& sample,NSNeuralShadowDataset::NSSampleBlockKind kind);

protected:
	template<class T>
	static bool ReadExact(std::span<const std::byte> bytes, T& output);

protected:
	static std::uint16_t ReadHalf(const std::vector<std::byte>& bytes, std::uint64_t index);
	
	static std::uint32_t GeometryInvalidReason(const NSRTGeometryCoverageState& geometry);

protected:
	static NSRTGeometryCoverageState NormalizedGeometry(NSRTGeometryCoverageState geometry);
	
	static NSRTQualityMetadata ExpectedQuality(const FDW_SD::Capture::NSCaptureSamplePlan& sample,const NSRTTargetCaptureEvidence& evidence);
	
	static std::uint32_t SamplesForLight(const FDW_SD::Capture::NSCaptureLightMetadata& light,std::uint32_t finiteSampleCount);

protected:
	static bool QualityEqual(const NSRTQualityMetadata& left, const NSRTQualityMetadata& right);
	static bool GeometryEqual(const NSRTGeometryCoverageState& left, const NSRTGeometryCoverageState& right);
	
	static std::vector<FDW_SD::Capture::NSCaptureOwnedBlock> LoadedToOwned(const NSNeuralShadowDataset::NSLoadedSample& loaded);
};

}
