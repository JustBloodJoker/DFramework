#include "pch.h"

#include <ShadowData/GroundTruthMetadata.h>

#include <ShadowData/NeuralShadowData.h>


namespace FDW_SD::GroundTruth {

NSRTTargetDispatchAddress ComputeRTTargetDispatchAddress(std::uint32_t workItemBase,std::uint32_t dispatchX,std::uint32_t dispatchY,std::uint32_t dispatchZ) {
	return { workItemBase + dispatchZ, dispatchY * 16u + dispatchX * 4u };
}

FDW_SD::Capture::NSCaptureResult GroundTruthMetadata::Failure(NSNeuralShadowDataset::NSDatasetErrorCode code, std::string message) {
	return { code, std::move(message) };
}

std::uint64_t GroundTruthMetadata::HashRevision(std::string_view revision) {
	auto hash = FNV_OFFSET;
	for (const auto character : revision) {
		hash = (hash ^ static_cast<unsigned char>(character)) * FNV_PRIME;
	}
		
	return hash == 0u ? 1u : hash;
}

const FDW_SD::Capture::NSCaptureOwnedBlock* GroundTruthMetadata::FindOwnedBlock(std::span<const FDW_SD::Capture::NSCaptureOwnedBlock> blocks, NSNeuralShadowDataset::NSSampleBlockKind kind) {
	for (const auto& block : blocks) if (block.Kind == kind) return &block;
		
	return nullptr;
}

const NSNeuralShadowDataset::NSLoadedSampleBlock* GroundTruthMetadata::FindLoadedBlock(const NSNeuralShadowDataset::NSLoadedSample& sample,NSNeuralShadowDataset::NSSampleBlockKind kind) {
	for (const auto& block : sample.Blocks) {
		if (block.Record.Kind == static_cast<std::uint32_t>(kind)) return &block;
	}
		
	return nullptr;
}

template<class T>
bool GroundTruthMetadata::ReadExact(std::span<const std::byte> bytes, T& output) {
	if (bytes.size() != sizeof(T)) return false;
		
	std::memcpy(&output, bytes.data(), sizeof(T));
	return true;
}

std::uint16_t GroundTruthMetadata::ReadHalf(const std::vector<std::byte>& bytes, std::uint64_t index) {
	std::uint16_t value = 0u;
	std::memcpy(&value, bytes.data() + index * sizeof(value), sizeof(value));
	return value;
}

std::uint32_t GroundTruthMetadata::GeometryInvalidReason(const NSRTGeometryCoverageState& geometry) {
	if (geometry.TLASComplete == 0u || geometry.MissingBLASInstances != 0u) {
		return static_cast<std::uint32_t>(FDW_SD::Output::NSOutputValidityReason::MissingAccelerationStructure);
	}
		
	if (geometry.ActiveTriangleInstances == 0u ||
		geometry.ConservativeAlphaOrOpacityInstances != 0u || geometry.ProceduralInstances != 0u) {
		return static_cast<std::uint32_t>(FDW_SD::Output::NSOutputValidityReason::UnsupportedGeometryCoverage);
	}
		
	return static_cast<std::uint32_t>(FDW_SD::Output::NSOutputValidityReason::Valid);
}

NSRTGeometryCoverageState GroundTruthMetadata::NormalizedGeometry(NSRTGeometryCoverageState geometry) {
	geometry.GroundTruthGeometryEligible = IsGroundTruthGeometryEligible(geometry) ? 1u : 0u;
	return geometry;
}

NSRTQualityMetadata GroundTruthMetadata::ExpectedQuality(const FDW_SD::Capture::NSCaptureSamplePlan& sample,const NSRTTargetCaptureEvidence& evidence) {
	auto settings = evidence.QualitySettings;
		
	settings.OfflineGenerationId = sample.GenerationId;
		
	auto quality = ShadowLightParameters::BuildQualityMetadata(settings,sample.GenerationId, NSRTOutputDomain::ScreenResolutionNSOutput);
		
	const auto geometry = NormalizedGeometry(evidence.Geometry);
	quality.GeometryCoverageFlags = geometry.Flags;
	quality.UnsupportedGeometryInstanceCount = geometry.ConservativeAlphaOrOpacityInstances + geometry.ProceduralInstances;
	quality.MissingAccelerationStructureInstanceCount = geometry.MissingBLASInstances + (geometry.TLASComplete != 0u ? 0u : 1u);
		
	if (geometry.GroundTruthGeometryEligible == 0u) quality.IsCanonicalGroundTruthPreset = 0u;
		
	return quality;
}

std::uint32_t GroundTruthMetadata::SamplesForLight(const FDW_SD::Capture::NSCaptureLightMetadata& light, std::uint32_t finiteSampleCount) {
	const auto softness = std::clamp(light.ShadowSoftness, 0.0f, 4.0f);
	if ((light.LightType == 0u || light.LightType == 1u) && light.SourceRadius * softness > 0.0f) {
		return finiteSampleCount;
	}
		
	if (light.LightType == 2u && light.DirectionalAngularRadius * softness > 0.0f) {
		return finiteSampleCount;
	}
		
	if (light.LightType == 3u && light.RectangleSize.X * softness > 0.0f &&
		light.RectangleSize.Y * softness > 0.0f) {
		return finiteSampleCount;
	}
		
	return NS_RT_DELTA_SAMPLE_COUNT;
}

bool GroundTruthMetadata::QualityEqual(const NSRTQualityMetadata& left, const NSRTQualityMetadata& right) {
	return std::memcmp(&left, &right, sizeof(left)) == 0;
}

bool GroundTruthMetadata::GeometryEqual(const NSRTGeometryCoverageState& left, const NSRTGeometryCoverageState& right) {
	return std::memcmp(&left, &right, sizeof(left)) == 0;
}

std::vector<FDW_SD::Capture::NSCaptureOwnedBlock> GroundTruthMetadata::LoadedToOwned(const NSNeuralShadowDataset::NSLoadedSample& loaded) {
	std::vector<FDW_SD::Capture::NSCaptureOwnedBlock> blocks;
		
	for (const auto& block : loaded.Blocks) {
		const auto kind = static_cast<NSNeuralShadowDataset::NSSampleBlockKind>(block.Record.Kind);
			
		if (kind == NSNeuralShadowDataset::NSSampleBlockKind::CaptureMetadata || kind == NSNeuralShadowDataset::NSSampleBlockKind::RTTargetMetadata) continue;
			
		blocks.push_back({ kind, block.Record.Version, block.Record.ElementCount, block.Bytes });
	}

	return blocks;
}

FDW_SD::Capture::NSCaptureResult GroundTruthMetadata::ValidateEvidence(const NSRTTargetCaptureEvidence& evidence) {
	if (evidence.EngineRevision.empty() || evidence.EngineRevision.size() > 256u) {
		return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::InvalidArgument, "a bounded engine revision is required for RT target capture");
	}

	const auto settings = ShadowLightParameters::SanitizeQualitySettings(evidence.QualitySettings);
	if (settings.Mode != NSRTQualityMode::OfflineGroundTruth ||
		settings.DenoisingPolicy != NSRTDenoisingPolicy::Disabled ||
		settings.PreviewFiniteEmitterSampleCount != evidence.QualitySettings.PreviewFiniteEmitterSampleCount) {
		return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::FormatMismatch, "RT target capture requires exact raw offline ground-truth settings");
	}

	return {};
}

FDW_SD::Capture::NSCaptureResult GroundTruthMetadata::ValidateBlocks(const FDW_SD::Capture::NSCaptureSamplePlan& sample,std::span<const FDW_SD::Capture::NSCaptureOwnedBlock> blocks,const NSRTTargetCaptureEvidence& evidence, NSRTTargetValidationStats& stats) {
	stats = {};
	auto result = ValidateEvidence(evidence);
	
	if (!result.Succeeded()) return result;
	
	if (evidence.QualitySettings.BaseSeed != sample.Metadata.BaseSeed) {
		return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::SampleMismatch, "RT base seed does not match deterministic capture metadata");
	}
	
	result = FDW_SD::Capture::CaptureConfiguration::ValidateGroundTruthInputBlocks(sample, blocks);
	
	if (!result.Succeeded()) return result;
	
	const auto* workHeaderBlock = FindOwnedBlock(blocks, NSNeuralShadowDataset::NSSampleBlockKind::WorkHeader);
	const auto* workItemsBlock = FindOwnedBlock(blocks, NSNeuralShadowDataset::NSSampleBlockKind::WorkItems);
	const auto* pixelBlock = FindOwnedBlock(blocks, NSNeuralShadowDataset::NSSampleBlockKind::PixelFeatures);
	const auto* targetBlock = FindOwnedBlock(blocks, NSNeuralShadowDataset::NSSampleBlockKind::RTTargetVisibility);
	const auto* validityBlock = FindOwnedBlock(blocks, NSNeuralShadowDataset::NSSampleBlockKind::TargetValidity);
	const auto* outputHeaderBlock = FindOwnedBlock(blocks, NSNeuralShadowDataset::NSSampleBlockKind::OutputHeader);
	
	if (!targetBlock || !validityBlock || !outputHeaderBlock) {
		return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::Incomplete, "ground truth requires target visibility, validity, and output header blocks");
	}
	
	FDW_SD::Work::NSWorkDispatchHeader work{};
	FDW_SD::Output::NSOutputHeader output{};
	
	if (!ReadExact(workHeaderBlock->Bytes, work) || !ReadExact(outputHeaderBlock->Bytes, output) ||
		outputHeaderBlock->Version != FDW_SD::Output::OUTPUT_LAYOUT_VERSION || outputHeaderBlock->ElementCount != 1u ||
		output.Producer != static_cast<std::uint32_t>(FDW_SD::Output::NSOutputProducer::RayTracedReference) ||
	
		!IsCompleteOutputForViewport(output, work, sample.GenerationId, sample.Metadata.OutputWidth, sample.Metadata.OutputHeight)) {
	
		return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::SampleMismatch, "RT output header is not complete for the screen-resolution output layout");
	}
	
	if (targetBlock->Version != FDW_SD::Output::OUTPUT_LAYOUT_VERSION ||
		targetBlock->ElementCount != output.OutputValueCount ||
		targetBlock->Bytes.size() != output.VisibilityByteCount ||
	
		validityBlock->Version != FDW_SD::Output::OUTPUT_LAYOUT_VERSION ||
		validityBlock->ElementCount != output.OutputValueCount ||
		validityBlock->Bytes.size() != output.ValidityByteCount) {
	
		return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::LayoutMismatch, "RT target R16/R8 blocks do not match the output header");
	}

	const auto geometryReason = GeometryInvalidReason(evidence.Geometry);
	const auto finiteSampleCount = ExpectedQuality(sample, evidence).FiniteEmitterSampleCount;
	for (std::uint32_t workIndex = 0u; workIndex < work.WorkItemCount; ++workIndex) {
		
		FDW_SD::Work::NSWorkItem item{};
		std::memcpy(&item, workItemsBlock->Bytes.data() + workIndex * sizeof(item), sizeof(item));
		
		if (item.LightIndex >= sample.Lights.size()) {
			return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::SampleMismatch, "RT target work item light is outside recorded capture lights");
		}
		
		const auto patchX = item.ScreenTileIndex % work.PatchCountX;
		const auto patchY = item.ScreenTileIndex / work.PatchCountX;
		
		for (std::uint32_t localPixel = 0u; localPixel < FDW_SD::Output::VALUES_PER_WORK_ITEM; ++localPixel) {
			
			const auto element = static_cast<std::uint64_t>(item.OutputOffset) + localPixel;
			const auto screenX = patchX * FDW_SD::Work::PATCH_WIDTH + localPixel % FDW_SD::Work::PATCH_WIDTH;
			const auto screenY = patchY * FDW_SD::Work::PATCH_HEIGHT + localPixel / FDW_SD::Work::PATCH_WIDTH;
			const auto reason = std::to_integer<std::uint8_t>(validityBlock->Bytes[element]);
		
			if (reason > static_cast<std::uint8_t>(FDW_SD::Output::NSOutputValidityReason::Incomplete)) {
				return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::LayoutMismatch, "RT target contains an unknown validity reason");
			}
			
			const auto receiverOffset = (static_cast<std::uint64_t>(workIndex) * FDW_SD::Features::PIXEL_CHANNEL_COUNT +
				static_cast<std::uint32_t>(FDW_SD::Features::NSPixelFeatureChannel::ReceiverValid)) * FDW_SD::Features::PIXELS_PER_PATCH + localPixel;
			
			const auto receiverValid = FDW_SD::Output::VisibilityEncodingTools::DecodeR16Float(ReadHalf(pixelBlock->Bytes, receiverOffset));
			
			std::uint32_t expectedReason = static_cast<std::uint32_t>(FDW_SD::Output::NSOutputValidityReason::Valid);
			
			if (screenX >= work.ScreenWidth || screenY >= work.ScreenHeight || receiverValid < 0.5f) {
				expectedReason = static_cast<std::uint32_t>(FDW_SD::Output::NSOutputValidityReason::PaddedOrNoReceiver);
			}
			else if (geometryReason != static_cast<std::uint32_t>(FDW_SD::Output::NSOutputValidityReason::Valid)) {
				expectedReason = geometryReason;
			}
			if (expectedReason != static_cast<std::uint32_t>(FDW_SD::Output::NSOutputValidityReason::Valid) && reason != expectedReason) {
				return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::SampleMismatch, "RT target validity does not mask padding/receiver/geometry state");
			}
			if (expectedReason == static_cast<std::uint32_t>(FDW_SD::Output::NSOutputValidityReason::Valid) &&
				(reason == static_cast<std::uint8_t>(FDW_SD::Output::NSOutputValidityReason::PaddedOrNoReceiver) ||
				 reason == static_cast<std::uint8_t>(FDW_SD::Output::NSOutputValidityReason::InvalidLightIdentity) ||
				 reason == static_cast<std::uint8_t>(FDW_SD::Output::NSOutputValidityReason::MissingAccelerationStructure) ||
				 reason == static_cast<std::uint8_t>(FDW_SD::Output::NSOutputValidityReason::UnsupportedGeometryCoverage) ||
				 reason >= static_cast<std::uint8_t>(FDW_SD::Output::NSOutputValidityReason::CapacityOverflow))) {
				
				return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::SampleMismatch, "complete RT target contains impossible identity/geometry/completion validity");
			}
			
			const auto encodedVisibility = ReadHalf(targetBlock->Bytes, element);
			const auto visibility = FDW_SD::Output::VisibilityEncodingTools::DecodeR16Float(encodedVisibility);
			if (!std::isfinite(visibility) || visibility < 0.0f || visibility > 1.0f) {
				return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::LayoutMismatch, "RT target visibility is not finite normalized R16");
			}
			
			if (reason == static_cast<std::uint8_t>(FDW_SD::Output::NSOutputValidityReason::Valid)) {
				++stats.ValidValueCount;
				stats.CompletedRayCount += SamplesForLight(sample.Lights[item.LightIndex], finiteSampleCount);
			}
			else {
				++stats.InvalidValueCount;
				
				if (encodedVisibility != FDW_SD::Output::VisibilityEncodingTools::EncodeR16Float(1.0f)) {
					return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::LayoutMismatch, "masked RT target elements must store canonical fully-lit payload");
				}
			}
		}
	}
	if (static_cast<std::uint64_t>(stats.ValidValueCount) + stats.InvalidValueCount != output.OutputValueCount) {
		return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::Incomplete, "RT target did not classify every output element exactly once");
	}
	const auto requiredValidTargets = FDW_SD::Capture::CaptureConfiguration::RequiredValidTargetCount(sample);
	if (stats.ValidValueCount < requiredValidTargets) {
		return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::Incomplete, "RT target contains fewer valid receiver values than the capture-plan minimum");
	}
	return {};
}

FDW_SD::Capture::NSCaptureResult GroundTruthMetadata::Build(const FDW_SD::Capture::NSCaptureSamplePlan& sample,const NSRTTargetCaptureEvidence& evidence, const NSRTTargetValidationStats& stats,std::vector<std::byte>& bytes) {
	bytes.clear();
	
	auto result = ValidateEvidence(evidence);
	
	if (!result.Succeeded()) return result;
	
	NSRTTargetMetadataHeader header{};
	header.HeaderByteSize = sizeof(header);
	header.FileByteSize = static_cast<std::uint32_t>(sizeof(header) + evidence.EngineRevision.size());
	header.SampleId = sample.SampleId;
	header.GenerationId = sample.GenerationId;
	header.EngineRevisionHash = HashRevision(evidence.EngineRevision);
	header.EngineRevisionOffset = sizeof(header);
	header.EngineRevisionByteSize = static_cast<std::uint32_t>(evidence.EngineRevision.size());
	header.OutputWidth = sample.Metadata.OutputWidth;
	header.OutputHeight = sample.Metadata.OutputHeight;
	header.WorkItemCount = stats.ValidValueCount + stats.InvalidValueCount;
	header.WorkItemCount /= FDW_SD::Output::VALUES_PER_WORK_ITEM;
	header.OutputValueCount = stats.ValidValueCount + stats.InvalidValueCount;
	header.ValidValueCount = stats.ValidValueCount;
	header.InvalidValueCount = stats.InvalidValueCount;
	header.CompletedRayCount = stats.CompletedRayCount;
	header.Quality = ExpectedQuality(sample, evidence);
	header.Geometry = NormalizedGeometry(evidence.Geometry);
	
	bytes.resize(header.FileByteSize);
	
	std::memcpy(bytes.data(), &header, sizeof(header));
	std::memcpy(bytes.data() + header.EngineRevisionOffset, evidence.EngineRevision.data(), evidence.EngineRevision.size());
	
	return {};
}

FDW_SD::Capture::NSCaptureResult GroundTruthMetadata::Validate(std::span<const std::byte> bytes,const FDW_SD::Capture::NSCaptureSamplePlan* expectedSample,const NSRTTargetCaptureEvidence* expectedEvidence,NSRTTargetMetadataHeader* outputHeader) {
	
	if (bytes.size() < sizeof(NSRTTargetMetadataHeader)) {
		return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::Truncated, "RT target metadata header is truncated");
	}
	
	NSRTTargetMetadataHeader header{};
	
	std::memcpy(&header, bytes.data(), sizeof(header));
	
	if (header.Magic != RT_TARGET_METADATA_MAGIC || header.Version != RT_TARGET_METADATA_VERSION ||
		header.HeaderByteSize != sizeof(header) || header.FileByteSize != bytes.size() ||
		header.EngineRevisionOffset != sizeof(header) || header.EngineRevisionByteSize == 0u ||
		static_cast<std::uint64_t>(header.EngineRevisionOffset) + header.EngineRevisionByteSize != bytes.size() ||
		header.SampleId == 0u || header.GenerationId == 0u || header.EngineRevisionHash == 0u ||
		header.OutputWidth == 0u || header.OutputHeight == 0u ||
		header.OutputValueCount != header.WorkItemCount * FDW_SD::Output::VALUES_PER_WORK_ITEM ||
		header.ValidValueCount == 0u ||
		header.ValidValueCount + header.InvalidValueCount != header.OutputValueCount ||
		header.OutputDomain != static_cast<std::uint32_t>(NSRTOutputDomain::ScreenResolutionNSOutput) ||
		header.VisibilityEncoding != static_cast<std::uint32_t>(FDW_SD::Output::NSVisibilityEncoding::R16Float) ||
		header.ValidityEncoding != static_cast<std::uint32_t>(FDW_SD::Output::NSValidityEncoding::R8Uint) ||
		header.MetadataFormatVersion != RT_TARGET_METADATA_FORMAT_VERSION) {
		return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::LayoutMismatch, "RT target metadata identity/layout is invalid");
	}
	
	for (const auto padding : header.Padding) {
		if (padding != 0u) {
			return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::LayoutMismatch, "RT target metadata padding is nonzero");
		}
	}
	
	const std::string revision(reinterpret_cast<const char*>(bytes.data() + header.EngineRevisionOffset),header.EngineRevisionByteSize);
	
	if (HashRevision(revision) != header.EngineRevisionHash) {
		return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::ChecksumMismatch, "RT target engine revision hash mismatch");
	}
	
	if (header.Quality.ReferenceVersion != NS_RT_REFERENCE_VERSION ||
		header.Quality.QualityMode != static_cast<std::uint32_t>(NSRTQualityMode::OfflineGroundTruth) ||
		header.Quality.FiniteEmitterSampleCount < NS_RT_OFFLINE_FINITE_SAMPLE_COUNT ||
		header.Quality.FiniteEmitterSampleCount > NS_RT_MAX_FINITE_SAMPLE_COUNT ||
		header.Quality.DenoisingPolicy != static_cast<std::uint32_t>(NSRTDenoisingPolicy::Disabled) ||
		header.Quality.IsOffline == 0u || header.Quality.AccumulationPrecisionBits != 32u ||
		header.Quality.TemporalAccumulationEnabled != 0u) {
		return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::FormatMismatch, "RT target metadata is not raw offline ground truth");
	}
	
	if (expectedSample && (header.SampleId != expectedSample->SampleId ||
		header.GenerationId != expectedSample->GenerationId ||
		header.OutputWidth != expectedSample->Metadata.OutputWidth ||
		header.OutputHeight != expectedSample->Metadata.OutputHeight ||
		header.Quality.BaseSeed != expectedSample->Metadata.BaseSeed ||
		(static_cast<std::uint64_t>(header.Quality.GenerationLow) |
		 (static_cast<std::uint64_t>(header.Quality.GenerationHigh) << 32u)) != expectedSample->GenerationId)) {
		return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::SampleMismatch, "RT target metadata does not match the planned sample");
	}
	
	if (expectedSample && header.ValidValueCount < FDW_SD::Capture::CaptureConfiguration::RequiredValidTargetCount(*expectedSample)) {
		return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::Incomplete, "RT target metadata is below the capture-plan valid-target minimum");
	}
	
	if (expectedSample && expectedEvidence) {
		const auto expectedQuality = ExpectedQuality(*expectedSample, *expectedEvidence);
		const auto expectedGeometry = NormalizedGeometry(expectedEvidence->Geometry);
		if (revision != expectedEvidence->EngineRevision || !QualityEqual(header.Quality, expectedQuality) ||
			!GeometryEqual(header.Geometry, expectedGeometry)) {
			return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::SampleMismatch, "RT target quality/geometry/revision evidence mismatch");
		}
	}
	
	if (outputHeader) *outputHeader = header;
	
	return {};
}

FDW_SD::Capture::NSCaptureResult GroundTruthMetadata::LoadPublished(const std::filesystem::path& datasetRoot,const std::filesystem::path& sampleRelativePath,const FDW_SD::Capture::NSCaptureSamplePlan* expectedSample,NSRTTargetMetadataHeader& header) {
	
	NSNeuralShadowDataset::NSLoadedSample loaded{};
	
	auto result = NSNeuralShadowDataset::DatasetContainer::LoadSampleFile(datasetRoot, sampleRelativePath, loaded);
	if (!result.Succeeded()) return Failure(result.Code, result.Message);
	
	const auto* metadata = FindLoadedBlock(loaded, NSNeuralShadowDataset::NSSampleBlockKind::RTTargetMetadata);
	if (!metadata) return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::Incomplete, "sample lacks RT target metadata");
	
	return Validate(metadata->Bytes, expectedSample, nullptr, &header);
}

FDW_SD::Capture::NSCaptureResult GroundTruthMetadata::ExecuteCapturePlan(const FDW_SD::Capture::NSCaptureConfig& config,const std::filesystem::path& datasetRoot,const NSRTTargetCaptureEvidence& evidence,const NSRTGroundTruthProvider& provider,FDW_SD::Capture::NSCaptureRunStats& stats) {
	
	stats = {};
	if (datasetRoot.empty() || !provider) {
		return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::InvalidArgument, "dataset root and RT ground-truth provider are required");
	}

	auto result = ValidateEvidence(evidence);
	if (!result.Succeeded()) return result;
	
	if (evidence.QualitySettings.BaseSeed != config.BaseSeed) {
		return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::SampleMismatch, "RT evidence base seed differs from capture config");
	}

	std::vector<FDW_SD::Capture::NSCaptureSamplePlan> plan;
	result = FDW_SD::Capture::CaptureConfiguration::BuildPlan(config, plan);
	
	if (!result.Succeeded()) return result;
	
	stats.Planned = static_cast<std::uint32_t>(plan.size());
	for (const auto& sample : plan) {
		NSNeuralShadowDataset::NSDatasetResult detail{};
		auto regeneration = NSNeuralShadowDataset::DatasetContainer::GetSampleRegenerationStatus(
			datasetRoot, config.DatasetId, sample.RelativePath, sample.SampleId, sample.GenerationId, detail);
		
		if (regeneration == NSNeuralShadowDataset::NSRegenerationStatus::Conflict) {
			return Failure(detail.Code, "ground-truth resume conflict: " + detail.Message);
		}
		
		if (regeneration == NSNeuralShadowDataset::NSRegenerationStatus::SkipValid) {
			NSNeuralShadowDataset::NSLoadedSample loaded{};
			detail = NSNeuralShadowDataset::DatasetContainer::LoadSampleFile(datasetRoot, sample.RelativePath, loaded);
		
			const auto* captureMetadata = FindLoadedBlock(loaded, NSNeuralShadowDataset::NSSampleBlockKind::CaptureMetadata);
			const auto* targetMetadata = FindLoadedBlock(loaded, NSNeuralShadowDataset::NSSampleBlockKind::RTTargetMetadata);
		
			auto existingBlocks = LoadedToOwned(loaded);
			NSRTTargetValidationStats existingStats{};
			if (detail.Succeeded() && captureMetadata && targetMetadata &&
				captureMetadata->Bytes == sample.MetadataBytes &&
		
				ValidateBlocks(sample, existingBlocks, evidence, existingStats).Succeeded() &&
				Validate(targetMetadata->Bytes, &sample, &evidence).Succeeded()) {
		
				++stats.Skipped;
				continue;
			}
		
			regeneration = NSNeuralShadowDataset::NSRegenerationStatus::RegenerateInvalid;
		}

		std::vector<FDW_SD::Capture::NSCaptureOwnedBlock> blocks;
		result = provider(sample, blocks);
		
		if (!result.Succeeded()) return result;
		
		NSRTTargetValidationStats targetStats{};
		result = ValidateBlocks(sample, blocks, evidence, targetStats);
		
		if (!result.Succeeded()) return result;
		
		std::vector<std::byte> targetMetadataBytes;
		result = Build(sample, evidence, targetStats, targetMetadataBytes);
		
		if (!result.Succeeded()) return result;
		
		blocks.push_back({ NSNeuralShadowDataset::NSSampleBlockKind::CaptureMetadata, FDW_SD::Capture::CAPTURE_METADATA_VERSION, 1u, sample.MetadataBytes });
		blocks.push_back({ NSNeuralShadowDataset::NSSampleBlockKind::RTTargetMetadata, RT_TARGET_METADATA_VERSION, 1u, std::move(targetMetadataBytes) });
		
		std::vector<NSNeuralShadowDataset::NSSampleBlockInput> inputs;
		inputs.reserve(blocks.size());
		
		for (const auto& block : blocks) inputs.push_back({ block.Kind, block.Version, block.ElementCount, block.Bytes });
		
		const auto publication = NSNeuralShadowDataset::DatasetContainer::PublishDatasetSample( datasetRoot, config.DatasetId, sample.RelativePath, sample.SampleId, sample.GenerationId, inputs);
		
		if (!publication.Succeeded()) return Failure(publication.Code, publication.Message);
		
		if (regeneration == NSNeuralShadowDataset::NSRegenerationStatus::RegenerateInvalid) ++stats.Regenerated;
		else ++stats.Generated;
	}
	return {};
}

}
