#include <ShadowData/CaptureConfiguration.h>
#include <ShadowData/NeuralShadowData.h>

class CaptureConfigurationTest {
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
		config.DatasetId = 0x2100'0001ull;
		config.BaseSeed = 0x1234'5678u;
		config.SampleCount = 4u;
		config.GenerationBase = 100u;
		config.SceneAsset = "procedural:receiver_plane";
		config.OutputWidth = 32u;
		config.OutputHeight = 16u;
		config.TargetSamplesPerPixel = 512u;
		config.GeometryCount = { 2u, 4u };
		config.GeometryTypeMask = 0x06u;
		config.LightCount = { 2u, 4u };
		config.LightTypeMask = 0x05u;
		config.LightAttenuationRadius = { 30.0f, 60.0f };
		return config;
	}

	static std::vector<FDW_SD::Capture::NSCaptureOwnedBlock> MakeInputBlocks( const FDW_SD::Capture::NSCaptureSamplePlan& sample) {
		
		FDW_SD::Work::NSWorkDispatchHeader header{};
		header.GenerationLow = static_cast<std::uint32_t>(sample.GenerationId);
		header.GenerationHigh = static_cast<std::uint32_t>(sample.GenerationId >> 32u);
		header.ScreenWidth = sample.Metadata.OutputWidth;
		header.ScreenHeight = sample.Metadata.OutputHeight;
		header.PatchCountX = FDW_SD::Work::ComputePatchCountX(header.ScreenWidth);
		header.PatchCountY = FDW_SD::Work::ComputePatchCountY(header.ScreenHeight);
		header.WorkItemCount = 1u;
		header.RequiredWorkItemCount = 1u;
		header.OutputValueCount = FDW_SD::Work::VALUES_PER_WORK_ITEM;
		
		FDW_SD::Work::NSWorkItem item{};
		item.LightIndex = 0u;
		item.ShadowMetaIndex = 0u;
		item.OutputOffset = 0u;
		
		const auto pixelElements = static_cast<std::uint64_t>(FDW_SD::Work::VALUES_PER_WORK_ITEM) * FDW_SD::Features::PIXEL_CHANNEL_COUNT;
		const auto lightElements = static_cast<std::uint64_t>(FDW_SD::Features::LIGHT_CHANNEL_COUNT);
		
		auto blocks = std::vector<FDW_SD::Capture::NSCaptureOwnedBlock>{
			{ NSNeuralShadowDataset::NSSampleBlockKind::WorkHeader, FDW_SD::Work::DATA_LAYOUT_VERSION, 1u, BytesOf(header) },
			{ NSNeuralShadowDataset::NSSampleBlockKind::WorkItems, FDW_SD::Work::DATA_LAYOUT_VERSION, 1u, BytesOf(item) },
			{ NSNeuralShadowDataset::NSSampleBlockKind::PixelFeatures, FDW_SD::Features::FEATURE_LAYOUT_VERSION, pixelElements,
				std::vector<std::byte>(pixelElements * sizeof(std::uint16_t)) },
			{ NSNeuralShadowDataset::NSSampleBlockKind::LightFeatures, FDW_SD::Features::FEATURE_LAYOUT_VERSION, lightElements,
				std::vector<std::byte>(lightElements * sizeof(std::uint16_t)) },
			{ NSNeuralShadowDataset::NSSampleBlockKind::HardInputVisibility, 1u, FDW_SD::Work::VALUES_PER_WORK_ITEM,
				std::vector<std::byte>(FDW_SD::Work::VALUES_PER_WORK_ITEM * sizeof(std::uint16_t)) }
		};
		
		auto* lightWords = reinterpret_cast<std::uint16_t*>(blocks[3].Bytes.data());
		lightWords[0] = 0x3c00u; // TypePoint = FP16 1.0; remaining  channels stay zero.
		return blocks;
	}

	static bool FlipLastByte(const std::filesystem::path& path) {
		
		std::fstream stream(path, std::ios::binary | std::ios::in | std::ios::out);
		if (!stream) return false;
		
		stream.seekg(-1, std::ios::end);
		char value = 0;
		stream.read(&value, 1);
		
		if (!stream) return false;
		
		value ^= 0x5a;
		stream.seekp(-1, std::ios::end);
		stream.write(&value, 1);
		
		return static_cast<bool>(stream);
	}

};

int CaptureConfigurationTest::Run() {
	const auto tempRoot = std::filesystem::temp_directory_path() / "fdw_capture_configuration";
	
	std::error_code filesystemError;
	std::filesystem::remove_all(tempRoot, filesystemError);
	std::filesystem::create_directories(tempRoot, filesystemError);
	
	if (filesystemError) return 1;

	const auto configPath = tempRoot / "capture.ini";
	{
		std::ofstream configFile(configPath);
		configFile <<
			"version=4\n"
			"dataset_id=553648129\n"
			"base_seed=305419896\n"
			"sample_count=4\n"
			"generation_base=100\n"
			"scene_asset=procedural:receiver_plane\n"
			"output_width=32\n"
			"output_height=16\n"
			"target_spp=512\n"
			"geometry_count_min=2\n"
			"geometry_count_max=4\n"
			"geometry_type_mask=6\n"
			"light_count_min=2\n"
			"light_count_max=4\n"
			"light_type_mask=5\n"
			"light_attenuation_min=30\n"
			"light_attenuation_max=60\n";
	}
	
	FDW_SD::Capture::NSCaptureConfig parsed{};
	auto result = FDW_SD::Capture::CaptureConfiguration::Load(configPath, parsed);
	if (!result.Succeeded() || parsed.DatasetId != MakeConfig().DatasetId || parsed.SampleCount != 4u ||
		parsed.TargetSamplesPerPixel != 512u) {
		std::cerr << "Config parsing failed: " << result.Message << '\n';
		return 2;
	}

	std::vector<FDW_SD::Capture::NSCaptureSamplePlan> firstPlan;
	std::vector<FDW_SD::Capture::NSCaptureSamplePlan> secondPlan;
	
	result = FDW_SD::Capture::CaptureConfiguration::BuildPlan(parsed, firstPlan);
	
	if (!result.Succeeded()) return 3;
	result = FDW_SD::Capture::CaptureConfiguration::BuildPlan(parsed, secondPlan);
	
	if (!result.Succeeded() || firstPlan.size() != 4u || firstPlan.size() != secondPlan.size()) return 4;
	
	for (std::size_t index = 0u; index < firstPlan.size(); ++index) {
		const auto& first = firstPlan[index];
		const auto& second = secondPlan[index];
		
		if (first.SampleId != second.SampleId || first.GenerationId != 100u + index ||
			first.SampleSeed != second.SampleSeed || first.RelativePath != second.RelativePath ||
			first.MetadataBytes != second.MetadataBytes || first.Metadata.FeatureLayoutVersion != 2u ||
			first.Metadata.PixelFeatureChannelCount != 10u || first.Metadata.LightFeatureChannelCount != 14u ||
			first.Metadata.Atlas.ShadowMetadataCount != first.Lights.size()) return 5;
		
		FDW_SD::Capture::NSCaptureMetadataHeader loadedMetadata{};
		
		result = FDW_SD::Capture::CaptureConfiguration::ValidateMetadata(first.MetadataBytes, &loadedMetadata);
		
		if (!result.Succeeded() || loadedMetadata.SampleId != first.SampleId ||
			loadedMetadata.GeometryCount != first.Geometry.size() || loadedMetadata.LightCount != first.Lights.size()) return 6;
		
		for (const auto& geometry : first.Geometry) if (geometry.Kind != 2u && geometry.Kind != 3u) return 6;
		
		for (const auto& light : first.Lights) if (light.LightType != 0u && light.LightType != 2u) return 6;
	}
	
	auto changedSeed = parsed;
	changedSeed.BaseSeed++;
	
	std::vector<FDW_SD::Capture::NSCaptureSamplePlan> changedPlan;
	result = FDW_SD::Capture::CaptureConfiguration::BuildPlan(changedSeed, changedPlan);
	
	if (!result.Succeeded() || changedPlan.front().MetadataBytes == firstPlan.front().MetadataBytes || changedPlan.front().SampleId == firstPlan.front().SampleId) {
		return 7;
	}

	auto changedSpp = parsed;
	changedSpp.TargetSamplesPerPixel = 1024u;
	result = FDW_SD::Capture::CaptureConfiguration::BuildPlan(changedSpp, changedPlan);
	if (!result.Succeeded() || changedPlan.front().SampleId == firstPlan.front().SampleId) return 7;

	auto sequenceCoverage = parsed;
	sequenceCoverage.DatasetId++;
	sequenceCoverage.BaseSeed = 4110u;
	sequenceCoverage.SampleCount = 100u;
	sequenceCoverage.GenerationBase = 1u;
	
	std::vector<FDW_SD::Capture::NSCaptureSamplePlan> sequencePlan;
	result = FDW_SD::Capture::CaptureConfiguration::BuildPlan(sequenceCoverage, sequencePlan);
	
	if (!result.Succeeded() || sequencePlan.size() != 100u ||
		sequencePlan[12].Metadata.ConfigVersion != FDW_SD::Capture::CAPTURE_CONFIG_FORMAT_VERSION ||
		sequencePlan[12].Metadata.RandomizationVersion != FDW_SD::Capture::CAPTURE_RANDOMIZATION_ALGORITHM_VERSION ||
		sequencePlan[12].SampleSeed != sequencePlan[13].SampleSeed ||
		sequencePlan[12].SceneAsset != sequencePlan[13].SceneAsset ||
		sequencePlan[12].Geometry.size() != sequencePlan[13].Geometry.size() ||
		std::memcmp(sequencePlan[12].Geometry.data(), sequencePlan[13].Geometry.data(),
			sequencePlan[12].Geometry.size() * sizeof(FDW_SD::Capture::NSCaptureGeometryMetadata)) != 0 ||
		sequencePlan[12].Lights.size() != sequencePlan[13].Lights.size() ||
		std::memcmp(sequencePlan[12].Lights.data(), sequencePlan[13].Lights.data(),
			sequencePlan[12].Lights.size() * sizeof(FDW_SD::Capture::NSCaptureLightMetadata)) != 0 ||
		sequencePlan[12].Metadata.Camera.Eye.X == sequencePlan[13].Metadata.Camera.Eye.X ||
		sequencePlan[12].Metadata.Camera.LookAt.X == sequencePlan[13].Metadata.Camera.LookAt.X ||
		FDW_SD::Capture::CaptureConfiguration::RequiredValidTargetCount(sequencePlan[12]) != FDW_SD::Capture::CAPTURE_MIN_VALID_TARGETS)
	{
		return 23;
	}
	
	auto incompleteSequenceCoverage = sequenceCoverage;
	incompleteSequenceCoverage.SampleCount = 99u;
	
	if (FDW_SD::Capture::CaptureConfiguration::Validate(incompleteSequenceCoverage).Succeeded()) return 24;
	
	auto incompatibleCoverage = sequenceCoverage;
	incompatibleCoverage.SceneAsset = "asset:Content/scene.obj";
	
	if (FDW_SD::Capture::CaptureConfiguration::Validate(incompatibleCoverage).Succeeded()) return 19;
	
	incompatibleCoverage = sequenceCoverage;
	incompatibleCoverage.LightAttenuationRadius = { 1.0f, 60.0f };
	
	if (FDW_SD::Capture::CaptureConfiguration::Validate(incompatibleCoverage).Succeeded()) return 20;

	std::uint32_t providerCalls = 0u;
	auto provider = [&providerCalls](const FDW_SD::Capture::NSCaptureSamplePlan& sample, std::vector<FDW_SD::Capture::NSCaptureOwnedBlock>& blocks) {
		++providerCalls;
		blocks = MakeInputBlocks(sample);
		return FDW_SD::Capture::NSCaptureResult{};
	};
	
	FDW_SD::Capture::NSCaptureRunStats stats{};
	
	const auto datasetRoot = tempRoot / "dataset";
	result = FDW_SD::Capture::CaptureConfiguration::ExecutePlan(parsed, datasetRoot, provider, stats);
	
	if (!result.Succeeded() || stats.Planned != 4u || stats.Generated != 4u ||
		stats.Regenerated != 0u || stats.Skipped != 0u || providerCalls != 4u) {
		std::cerr << "First unattended run failed: " << result.Message << '\n';
		return 8;
	}

	providerCalls = 0u;
	result = FDW_SD::Capture::CaptureConfiguration::ExecutePlan(parsed, datasetRoot, provider, stats);
	if (!result.Succeeded() || stats.Skipped != 4u || stats.Generated != 0u || providerCalls != 0u) {
		std::cerr << "Resume did not skip valid samples: " << result.Message << '\n';
		return 9;
	}

	NSNeuralShadowDataset::NSLoadedDatasetManifest manifest{};
	auto datasetResult = NSNeuralShadowDataset::DatasetContainer::LoadDatasetManifest(datasetRoot, manifest);
	
	if (!datasetResult.Succeeded() || manifest.Entries.size() != 4u) return 10;
	
	NSNeuralShadowDataset::NSLoadedSample loadedSample{};
	datasetResult = NSNeuralShadowDataset::DatasetContainer::LoadSampleFile(datasetRoot, firstPlan[0].RelativePath, loadedSample);
	
	if (!datasetResult.Succeeded() || loadedSample.Blocks.size() != 6u) return 11;
	const auto metadataBlock = std::find_if(loadedSample.Blocks.begin(), loadedSample.Blocks.end(), [](const auto& block) {
		return block.Record.Kind == static_cast<std::uint32_t>(NSNeuralShadowDataset::NSSampleBlockKind::CaptureMetadata);
	});
	
	if (metadataBlock == loadedSample.Blocks.end() || metadataBlock->Bytes != firstPlan[0].MetadataBytes) return 12;

	if (!FlipLastByte(datasetRoot / firstPlan[1].RelativePath)) return 13;
	
	providerCalls = 0u;
	result = FDW_SD::Capture::CaptureConfiguration::ExecutePlan(parsed, datasetRoot, provider, stats);
	
	if (!result.Succeeded() || stats.Regenerated != 1u || stats.Skipped != 3u || providerCalls != 1u) {
		std::cerr << "Interrupted/corrupt resume failed: " << result.Message << '\n';
		return 14;
	}

	auto forbiddenBlocks = MakeInputBlocks(firstPlan.front());
	forbiddenBlocks.push_back({ NSNeuralShadowDataset::NSSampleBlockKind::RTTargetVisibility, 1u, 1u, std::vector<std::byte>(2u) });
	result = FDW_SD::Capture::CaptureConfiguration::ValidateInputBlocks(firstPlan.front(), forbiddenBlocks);
	
	if (result.Code != NSNeuralShadowDataset::NSDatasetErrorCode::InvalidArgument) return 15;
	
	auto wrongShape = MakeInputBlocks(firstPlan.front());
	wrongShape[2].Bytes.pop_back();
	result = FDW_SD::Capture::CaptureConfiguration::ValidateInputBlocks(firstPlan.front(), wrongShape);
	
	if (result.Code != NSNeuralShadowDataset::NSDatasetErrorCode::LayoutMismatch) return 16;

	// All owned tensor blocks share the same version/count/byte-size contract.
	for (std::size_t blockIndex = 1u; blockIndex < 5u; ++blockIndex) {
		for (unsigned field = 0u; field < 3u; ++field) {
			auto malformed = MakeInputBlocks(firstPlan.front());
			auto& block = malformed[blockIndex];
			
			if (field == 0u) ++block.Version;
			else if (field == 1u) ++block.ElementCount;
			else block.Bytes.pop_back();

			result = FDW_SD::Capture::CaptureConfiguration::ValidateInputBlocks(firstPlan.front(), malformed);
			
			if (result.Code != NSNeuralShadowDataset::NSDatasetErrorCode::LayoutMismatch ||
				result.Message != "capture tensor bytes/elements do not match the active feature shapes") {
				return 25;
			}
		}
	}

	auto invalidPixels = MakeInputBlocks(firstPlan.front());
	reinterpret_cast<std::uint16_t*>(invalidPixels[2].Bytes.data())[0] = 0x3c00u;
	result = FDW_SD::Capture::CaptureConfiguration::ValidateInputBlocks(firstPlan.front(), invalidPixels);
	if (result.Message != "pixel feature value violates the active bounds or padding") return 26;

	auto invalidLight = MakeInputBlocks(firstPlan.front());
	reinterpret_cast<std::uint16_t*>(invalidLight[3].Bytes.data())[1] = 0x3c00u;
	result = FDW_SD::Capture::CaptureConfiguration::ValidateInputBlocks(firstPlan.front(), invalidLight);
	if (result.Message != "light feature must have one active type") return 27;

	auto duplicate = MakeInputBlocks(firstPlan.front());
	duplicate.push_back(duplicate[0]);
	result = FDW_SD::Capture::CaptureConfiguration::ValidateInputBlocks(firstPlan.front(), duplicate);
	if (result.Code != NSNeuralShadowDataset::NSDatasetErrorCode::DuplicateBlock ||
		result.Message != "capture input block kinds must be unique") return 28;

	std::filesystem::remove_all(tempRoot, filesystemError);
	return 0;
}

int main() {
	return CaptureConfigurationTest::Run();
}
