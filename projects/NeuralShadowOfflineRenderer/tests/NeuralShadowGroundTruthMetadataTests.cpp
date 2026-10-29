#include <ShadowData/GroundTruthMetadata.h>
#include <ShadowData/NeuralShadowData.h>
#include <ShadowData/NeuralShadowData.h>
#include <D3DFramework/Utilites/Structures.h>

class GroundTruthMetadataTest {
public:
	static int Run();

protected:

template<class T>
static std::vector<std::byte> BytesOf(const T& value) {
	const auto bytes = std::as_bytes(std::span<const T>(&value, 1u));
	return { bytes.begin(), bytes.end() };
}

static FDW_SD::Capture::NSCaptureConfig MakeConfig() {
	FDW_SD::Capture::NSCaptureConfig config{};
	config.DatasetId = 0x2200'0001ull;
	config.BaseSeed = 0x1020'3040u;
	config.SampleCount = 2u;
	config.GenerationBase = 0x1'0000'0022ull;
	config.SceneAsset = "procedural:rt_target_fixture";
	config.OutputWidth = 16u;
	config.OutputHeight = 16u;
	config.TargetSamplesPerPixel = 512u;
	config.GeometryCount = { 1u, 1u };
	config.LightCount = { 2u, 2u };
	config.LightAttenuationRadius = { 30.0f, 30.0f };
	return config;
}

static FDW_SD::GroundTruth::NSRTTargetCaptureEvidence MakeEvidence() {
	FDW_SD::GroundTruth::NSRTTargetCaptureEvidence evidence{};
	evidence.EngineRevision = "rt-target-test-revision";
	evidence.QualitySettings.Mode = NSRTQualityMode::OfflineGroundTruth;
	evidence.QualitySettings.BaseSeed = MakeConfig().BaseSeed;
	evidence.QualitySettings.PreviewFiniteEmitterSampleCount = MakeConfig().TargetSamplesPerPixel;
	evidence.QualitySettings.DenoisingPolicy = NSRTDenoisingPolicy::Disabled;
	evidence.Geometry.Flags = ToGeometryCoverageBits(NSRTGeometryCoverageFlags::OpaqueTriangles);
	evidence.Geometry.ActiveTriangleInstances = 1u;
	evidence.Geometry.RigidInstances = 1u;
	evidence.Geometry.TLASComplete = 1u;
	return evidence;
}

static std::vector<FDW_SD::Capture::NSCaptureOwnedBlock> MakeBlocks(const FDW_SD::Capture::NSCaptureSamplePlan& sample) {
	
	FDW_SD::Work::NSWorkDispatchHeader work{};
	work.GenerationLow = static_cast<std::uint32_t>(sample.GenerationId);
	work.GenerationHigh = static_cast<std::uint32_t>(sample.GenerationId >> 32u);
	work.ScreenWidth = 16u;
	work.ScreenHeight = 16u;
	work.PatchCountX = 1u;
	work.PatchCountY = 1u;
	work.WorkItemCount = 1u;
	work.RequiredWorkItemCount = 1u;
	work.OutputValueCount = FDW_SD::Output::VALUES_PER_WORK_ITEM;
	
	FDW_SD::Work::NSWorkItem item{};
	item.LightIndex = 0u;
	item.OutputOffset = 0u;
	
	const auto output = MakeCompleteOutputHeader(sample.GenerationId, 1u, FDW_SD::Output::NSOutputProducer::RayTracedReference);
	const auto pixelElements = std::uint64_t{ FDW_SD::Output::VALUES_PER_WORK_ITEM } * FDW_SD::Features::PIXEL_CHANNEL_COUNT;
	const auto lightElements = std::uint64_t{ FDW_SD::Features::LIGHT_CHANNEL_COUNT };
	
	std::vector<std::byte> pixels(pixelElements * sizeof(std::uint16_t));
	
	auto* pixelWords = reinterpret_cast<std::uint16_t*>(pixels.data());
	
	const auto receiverBase = static_cast<std::uint32_t>(FDW_SD::Features::NSPixelFeatureChannel::ReceiverValid) * FDW_SD::Features::PIXELS_PER_PATCH;
	
	for (std::uint32_t i = 0u; i < FDW_SD::Features::PIXELS_PER_PATCH; ++i) pixelWords[receiverBase + i] = 0x3c00u;
	
	std::vector<std::byte> target(FDW_SD::Output::VALUES_PER_WORK_ITEM * sizeof(std::uint16_t));
	
	auto* targetWords = reinterpret_cast<std::uint16_t*>(target.data());
	
	for (std::uint32_t i = 0u; i < FDW_SD::Output::VALUES_PER_WORK_ITEM; ++i) targetWords[i] = 0x3800u;
	
	std::vector<std::byte> hard(FDW_SD::Output::VALUES_PER_WORK_ITEM * sizeof(std::uint16_t), std::byte{ 0 });
	std::vector<std::byte> light(lightElements * sizeof(std::uint16_t), std::byte{ 0 });
	
	reinterpret_cast<std::uint16_t*>(light.data())[0] = 0x3c00u;
	
	return {
		{ NSNeuralShadowDataset::NSSampleBlockKind::WorkHeader, FDW_SD::Work::DATA_LAYOUT_VERSION, 1u, BytesOf(work) },
		{ NSNeuralShadowDataset::NSSampleBlockKind::WorkItems, FDW_SD::Work::DATA_LAYOUT_VERSION, 1u, BytesOf(item) },
		{ NSNeuralShadowDataset::NSSampleBlockKind::PixelFeatures, FDW_SD::Features::FEATURE_LAYOUT_VERSION, pixelElements, std::move(pixels) },
		{ NSNeuralShadowDataset::NSSampleBlockKind::LightFeatures, FDW_SD::Features::FEATURE_LAYOUT_VERSION, lightElements, std::move(light) },
		{ NSNeuralShadowDataset::NSSampleBlockKind::HardInputVisibility, FDW_SD::Output::OUTPUT_LAYOUT_VERSION, FDW_SD::Output::VALUES_PER_WORK_ITEM, std::move(hard) },
		{ NSNeuralShadowDataset::NSSampleBlockKind::RTTargetVisibility, FDW_SD::Output::OUTPUT_LAYOUT_VERSION, FDW_SD::Output::VALUES_PER_WORK_ITEM, std::move(target) },
		{ NSNeuralShadowDataset::NSSampleBlockKind::TargetValidity, FDW_SD::Output::OUTPUT_LAYOUT_VERSION, FDW_SD::Output::VALUES_PER_WORK_ITEM,
			std::vector<std::byte>(FDW_SD::Output::VALUES_PER_WORK_ITEM, std::byte{ 0 }) },
		{ NSNeuralShadowDataset::NSSampleBlockKind::OutputHeader, FDW_SD::Output::OUTPUT_LAYOUT_VERSION, 1u, BytesOf(output) }
	};
}

};

int GroundTruthMetadataTest::Run() {
	const auto opaqueTextured = ClassifyGeometryMaterial({NSRTGeometryAlphaMode::Opaque, 1u, 0u, 1u, 1u, 0.5f, 1.0f });
	const auto alphaMask = ClassifyGeometryMaterial({NSRTGeometryAlphaMode::Mask, 1u, 0u, 1u, 1u, 0.5f, 1.0f });
	const auto missingMaskTexture = ClassifyGeometryMaterial({NSRTGeometryAlphaMode::Mask, 0u, 0u, 1u, 1u, 0.5f, 1.0f });
	const auto blended = ClassifyGeometryMaterial({NSRTGeometryAlphaMode::Blend, 1u, 0u, 1u, 1u, 0.5f, 0.5f });
	
	if (opaqueTextured != NSRTGeometrySupportClass::OpaqueTriangles ||
		alphaMask != NSRTGeometrySupportClass::AlphaTestedTriangles ||
		missingMaskTexture != NSRTGeometrySupportClass::UnsupportedAlphaOrOpacity ||
		blended != NSRTGeometrySupportClass::UnsupportedAlphaOrOpacity)
	{
		return 20;
	}
	
	std::vector<FDW_SD::Capture::NSCaptureSamplePlan> plan;
	auto result = FDW_SD::Capture::CaptureConfiguration::BuildPlan(MakeConfig(), plan);
	
	if (!result.Succeeded() || plan.size() != 2u) return 1;
	auto blocks = MakeBlocks(plan.front());
	auto evidence = MakeEvidence();
	bool localPixels[256]{};
	
	for (std::uint32_t y = 0u; y < 16u; ++y) {
		for (std::uint32_t x = 0u; x < 4u; ++x) {
			
			const auto address = FDW_SD::GroundTruth::ComputeRTTargetDispatchAddress(11u, x, y, 2u);
			if (address.WorkItemIndex != 13u || address.FirstLocalPixel > 252u) return 2;
			
			for (std::uint32_t lane = 0u; lane < 4u; ++lane) {
				if (localPixels[address.FirstLocalPixel + lane]) return 2;
				localPixels[address.FirstLocalPixel + lane] = true;
			}
		}
	}
	
	for (const auto seen : localPixels) if (!seen) return 2;
	
	FDW_SD::GroundTruth::NSRTTargetValidationStats validation{};
	result = FDW_SD::GroundTruth::GroundTruthMetadata::ValidateBlocks(plan.front(), blocks, evidence, validation);
	
	if (!result.Succeeded() || validation.ValidValueCount != 256u || validation.InvalidValueCount != 0u ||
		validation.CompletedRayCount != 256u * MakeConfig().TargetSamplesPerPixel) {
		
		std::cerr << result.Message << '\n';
		return 3;
	}
	
	std::vector<std::byte> metadata;
	result = FDW_SD::GroundTruth::GroundTruthMetadata::Build(plan.front(), evidence, validation, metadata);
	FDW_SD::GroundTruth::NSRTTargetMetadataHeader header{};
	
	if (!result.Succeeded() || !FDW_SD::GroundTruth::GroundTruthMetadata::Validate(metadata, &plan.front(), &evidence, &header).Succeeded() ||
		header.OutputDomain != static_cast<std::uint32_t>(NSRTOutputDomain::ScreenResolutionNSOutput) ||
		header.Quality.IsCanonicalGroundTruthPreset == 0u || header.Quality.FiniteEmitterSampleCount != 512u)
	{
		return 4;
	}

	auto invalid = blocks;
	auto& validity = invalid[6].Bytes;
	validity[7] = static_cast<std::byte>(static_cast<std::uint8_t>(FDW_SD::Output::NSOutputValidityReason::PaddedOrNoReceiver));
	
	if (FDW_SD::GroundTruth::GroundTruthMetadata::ValidateBlocks(plan.front(), invalid, evidence, validation).Succeeded()) return 5;
	
	auto atlasHeader = *reinterpret_cast<FDW_SD::Output::NSOutputHeader*>(invalid[7].Bytes.data());
	atlasHeader.Producer = static_cast<std::uint32_t>(FDW_SD::Output::NSOutputProducer::DeterministicBaseline);
	invalid[7].Bytes = BytesOf(atlasHeader);
	
	if (FDW_SD::GroundTruth::GroundTruthMetadata::ValidateBlocks(plan.front(), invalid, evidence, validation).Succeeded()) return 6;
	
	auto emptyReceiver = blocks;
	auto* emptyPixels = reinterpret_cast<std::uint16_t*>(emptyReceiver[2].Bytes.data());
	const auto receiverBase = static_cast<std::uint32_t>(FDW_SD::Features::NSPixelFeatureChannel::ReceiverValid) * FDW_SD::Features::PIXELS_PER_PATCH;
	
	for (std::uint32_t index = 0u; index < FDW_SD::Features::PIXELS_PER_PATCH; ++index) {
		emptyPixels[receiverBase + index] = 0u;
		reinterpret_cast<std::uint16_t*>(emptyReceiver[5].Bytes.data())[index] = 0x3c00u;
		emptyReceiver[6].Bytes[index] = static_cast<std::byte>(static_cast<std::uint8_t>(FDW_SD::Output::NSOutputValidityReason::PaddedOrNoReceiver));
	}
	
	if (FDW_SD::GroundTruth::GroundTruthMetadata::ValidateBlocks(plan.front(), emptyReceiver, evidence, validation).Succeeded()) return 7;

	const auto tempRoot = std::filesystem::temp_directory_path() / "fdw_rt_target";
	std::error_code error;
	std::filesystem::remove_all(tempRoot, error);
	std::uint32_t calls = 0u;
	
	auto provider = [&calls](const FDW_SD::Capture::NSCaptureSamplePlan& sample, std::vector<FDW_SD::Capture::NSCaptureOwnedBlock>& outputBlocks) {
		++calls;
		outputBlocks = MakeBlocks(sample);
		return FDW_SD::Capture::NSCaptureResult{};
	};
	
	FDW_SD::Capture::NSCaptureRunStats stats{};
	result = FDW_SD::GroundTruth::GroundTruthMetadata::ExecuteCapturePlan(MakeConfig(), tempRoot, evidence, provider, stats);
	
	if (!result.Succeeded() || calls != 2u || stats.Generated != 2u) return 8;
	
	calls = 0u;
	result = FDW_SD::GroundTruth::GroundTruthMetadata::ExecuteCapturePlan(MakeConfig(), tempRoot, evidence, provider, stats);
	
	if (!result.Succeeded() || calls != 0u || stats.Skipped != 2u) return 9;
	
	std::filesystem::remove_all(tempRoot, error);
	return 0;
}

int main() {
	return GroundTruthMetadataTest::Run();
}
