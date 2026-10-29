#include <ShadowData/DatasetSplit.h>

#include <ShadowData/CaptureConfiguration.h>
#include <ShadowData/NeuralShadowData.h>
#include <ShadowData/GroundTruthMetadata.h>
#include <ShadowData/NeuralShadowData.h>

class DatasetSplitTest {
public:
	static int Run();

protected:

template<class T>
static std::vector<std::byte> BytesOf(const T& value) {
	const auto bytes = std::as_bytes(std::span<const T>(&value, 1u));
	return { bytes.begin(), bytes.end() };
}

static std::uint64_t HashGroup(std::uint64_t seed, std::uint64_t group) {
	std::uint64_t hash = 14695981039346656037ull;
	
	for (unsigned shift = 0u; shift < 64u; shift += 8u) hash =
		(hash ^ static_cast<std::uint8_t>(seed >> shift)) * 1099511628211ull;
	
	for (unsigned shift = 0u; shift < 64u; shift += 8u) hash =
		(hash ^ static_cast<std::uint8_t>(group >> shift)) * 1099511628211ull;
	
	return hash;
}

static std::uint64_t FindGroup(std::uint64_t seed, bool validation) {
	for (std::uint64_t group = 1u; group < 100'000u; ++group) {
		if ((HashGroup(seed, group) % 10'000u < 5'000u) == validation) return group;
	}
	
	return 0u;
}

static FDW_SD::Capture::NSCaptureConfig MakeCaptureConfig(std::uint64_t datasetId,std::string scene,std::uint32_t count,std::uint64_t generationBase,std::uint32_t seed) {
	
	FDW_SD::Capture::NSCaptureConfig config{};
	config.DatasetId = datasetId;
	config.BaseSeed = seed;
	config.SampleCount = count;
	config.GenerationBase = generationBase;
	config.SceneAsset = "procedural:" + std::move(scene);
	config.OutputWidth = 16u;
	config.OutputHeight = 16u;
	config.GeometryCount = { 1u, 1u };
	config.LightCount = { 2u, 2u };
	config.LightAttenuationRadius = { 30.0f, 30.0f };
	config.LightSourceRadius = { 0.0f, 0.0f };
	config.DirectionalAngularRadius = { 0.0f, 0.0f };
	
	return config;
}

static bool PublishFixtureSample(const std::filesystem::path& root,std::uint64_t datasetId,const FDW_SD::Capture::NSCaptureSamplePlan& sample) {
	std::vector<std::byte> target(256u * sizeof(std::uint16_t));
	
	auto* words = reinterpret_cast<std::uint16_t*>(target.data());
	for (std::uint32_t index = 0u; index < 256u; ++index) {
		const float value = index < 64u ? 0.0f : index < 128u ? 0.5f : 1.0f;
		words[index] = FDW_SD::Output::VisibilityEncodingTools::EncodeR16Float(value);
	}
	
	std::vector<std::byte> validity(256u, std::byte{ 0 });
	validity.back() = static_cast<std::byte>(static_cast<std::uint8_t>(FDW_SD::Output::NSOutputValidityReason::PaddedOrNoReceiver));
	
	words[255] = FDW_SD::Output::VisibilityEncodingTools::EncodeR16Float(1.0f);
	const auto output = MakeCompleteOutputHeader(sample.GenerationId, 1u, FDW_SD::Output::NSOutputProducer::RayTracedReference);
	
	FDW_SD::GroundTruth::NSRTTargetCaptureEvidence evidence{};
	evidence.EngineRevision = "split-fixture";
	evidence.QualitySettings.Mode = NSRTQualityMode::OfflineGroundTruth;
	evidence.QualitySettings.BaseSeed = sample.Metadata.BaseSeed;
	evidence.QualitySettings.PreviewFiniteEmitterSampleCount = NS_RT_OFFLINE_FINITE_SAMPLE_COUNT;
	evidence.Geometry.Flags = ToGeometryCoverageBits(NSRTGeometryCoverageFlags::OpaqueTriangles);
	evidence.Geometry.ActiveTriangleInstances = 1u;
	evidence.Geometry.RigidInstances = 1u;
	evidence.Geometry.TLASComplete = 1u;
	
	FDW_SD::GroundTruth::NSRTTargetValidationStats stats{ 255u, 1u, 255u };
	std::vector<std::byte> rtMetadata;
	
	if (!FDW_SD::GroundTruth::GroundTruthMetadata::Build(sample, evidence, stats, rtMetadata).Succeeded()) return false;
	
	const auto captureMetadata = sample.MetadataBytes;
	const auto outputBytes = BytesOf(output);
	const std::vector<NSNeuralShadowDataset::NSSampleBlockInput> blocks{
		{ NSNeuralShadowDataset::NSSampleBlockKind::CaptureMetadata, FDW_SD::Capture::CAPTURE_METADATA_VERSION, 1u, captureMetadata },
		{ NSNeuralShadowDataset::NSSampleBlockKind::RTTargetVisibility, FDW_SD::Output::OUTPUT_LAYOUT_VERSION, 256u, target },
		{ NSNeuralShadowDataset::NSSampleBlockKind::TargetValidity, FDW_SD::Output::OUTPUT_LAYOUT_VERSION, 256u, validity },
		{ NSNeuralShadowDataset::NSSampleBlockKind::OutputHeader, FDW_SD::Output::OUTPUT_LAYOUT_VERSION, 1u, outputBytes },
		{ NSNeuralShadowDataset::NSSampleBlockKind::RTTargetMetadata, FDW_SD::GroundTruth::RT_TARGET_METADATA_VERSION, 1u, rtMetadata }
	};
	
	const auto path = std::filesystem::path("samples") / (std::to_string(sample.SampleId) + ".nssample");
	return NSNeuralShadowDataset::DatasetContainer::PublishDatasetSample(root, datasetId, path, sample.SampleId, sample.GenerationId, blocks).Succeeded();
}

static bool ReadBytes(const std::filesystem::path& path, std::vector<std::byte>& bytes) {
	
	std::error_code error;
	
	const auto size = std::filesystem::file_size(path, error);
	
	if (error) return false;
	
	bytes.resize(static_cast<std::size_t>(size));
	std::ifstream stream(path, std::ios::binary);
	stream.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
	
	return static_cast<bool>(stream);
}

static std::string CoverageText(std::uint32_t flags) {
	std::string text;
	const char* names[] = { "contact", "thin", "large_penumbra", "motion", "ood", "stress" };
	for (std::uint32_t bit = 0u; bit < 6u; ++bit) {
	
		if ((flags & (1u << bit)) == 0u) continue;
	
		if (!text.empty()) text += '|';
		text += names[bit];
	}
	
	return text.empty() ? "none" : text;
}

};

int DatasetSplitTest::Run() {
	
	constexpr std::uint64_t datasetId = 0x2300'0001ull;
	constexpr std::uint64_t splitSeed = 0x2300'0023ull;
	const auto temp = std::filesystem::temp_directory_path() / "fdw_dataset_split";
	
	std::error_code error;
	std::filesystem::remove_all(temp, error);
	
	const auto dataset = temp / "dataset";
	std::vector<FDW_SD::Capture::NSCaptureSamplePlan> development;
	std::vector<FDW_SD::Capture::NSCaptureSamplePlan> unseen;
	std::vector<FDW_SD::Capture::NSCaptureSamplePlan> stress;
	
	if (!FDW_SD::Capture::CaptureConfiguration::BuildPlan(MakeCaptureConfig(datasetId, "dev", 6u, 100u, 11u), development).Succeeded() ||
		!FDW_SD::Capture::CaptureConfiguration::BuildPlan(MakeCaptureConfig(datasetId, "ood", 2u, 200u, 22u), unseen).Succeeded() ||
		!FDW_SD::Capture::CaptureConfiguration::BuildPlan(MakeCaptureConfig(datasetId, "stress", 2u, 300u, 33u), stress).Succeeded()) return 1;
	
	for (const auto& sample : development) if (!PublishFixtureSample(dataset, datasetId, sample)) return 2;
	
	for (const auto& sample : unseen) if (!PublishFixtureSample(dataset, datasetId, sample)) return 3;
	
	for (const auto& sample : stress) if (!PublishFixtureSample(dataset, datasetId, sample)) return 3;

	const auto trainGroup = FindGroup(splitSeed, false);
	const auto validationGroup = FindGroup(splitSeed, true);
	
	if (trainGroup == 0u || validationGroup == 0u || trainGroup == validationGroup) return 4;
	
	std::vector<FDW_SD::Split::NSSplitCatalogRecord> catalog{
		{ development[0].SampleId, development[0].GenerationId, trainGroup, 5001u, 1u, 10u,
			static_cast<std::uint32_t>(FDW_SD::Split::NSCoverageFlags::ContactShadow) | static_cast<std::uint32_t>(FDW_SD::Split::NSCoverageFlags::Motion) },
		{ development[1].SampleId, development[1].GenerationId, trainGroup, 5001u, 1u, 11u,
			static_cast<std::uint32_t>(FDW_SD::Split::NSCoverageFlags::ContactShadow) | static_cast<std::uint32_t>(FDW_SD::Split::NSCoverageFlags::Motion) },
		{ development[2].SampleId, development[2].GenerationId, validationGroup, 0u, 1u, 0u,
			static_cast<std::uint32_t>(FDW_SD::Split::NSCoverageFlags::ThinGeometry) },
		{ development[3].SampleId, development[3].GenerationId, trainGroup, 0u, 1u, 0u,
			static_cast<std::uint32_t>(FDW_SD::Split::NSCoverageFlags::LargePenumbra) },
		{ development[4].SampleId, development[4].GenerationId, validationGroup, 0u, 1u, 0u,
			static_cast<std::uint32_t>(FDW_SD::Split::NSCoverageFlags::ContactShadow) },
		{ development[5].SampleId, development[5].GenerationId, validationGroup, 0u, 1u, 0u,
			static_cast<std::uint32_t>(FDW_SD::Split::NSCoverageFlags::ThinGeometry) },
		{ unseen[0].SampleId, unseen[0].GenerationId, 9001u, 0u, 2u, 0u,
			static_cast<std::uint32_t>(FDW_SD::Split::NSCoverageFlags::OutOfDistribution) },
		{ unseen[1].SampleId, unseen[1].GenerationId, 9001u, 0u, 2u, 0u,
			static_cast<std::uint32_t>(FDW_SD::Split::NSCoverageFlags::OutOfDistribution) },
		{ stress[0].SampleId, stress[0].GenerationId, 9002u, 0u, 3u, 0u,
			static_cast<std::uint32_t>(FDW_SD::Split::NSCoverageFlags::StressCase) |
			static_cast<std::uint32_t>(FDW_SD::Split::NSCoverageFlags::ThinGeometry) },
		{ stress[1].SampleId, stress[1].GenerationId, 9002u, 0u, 3u, 0u,
			static_cast<std::uint32_t>(FDW_SD::Split::NSCoverageFlags::StressCase) |
			static_cast<std::uint32_t>(FDW_SD::Split::NSCoverageFlags::ThinGeometry) }
	};
	
	const auto catalogPath = temp / "catalog.csv";
	{
		std::ofstream stream(catalogPath);
		stream << "sample_id,generation_id,leakage_group_id,sequence_id,frame_index,domain_id,coverage_flags\n";
		for (const auto& record : catalog) {
			stream << record.SampleId << ',' << record.GenerationId << ',' << record.LeakageGroupId << ','
				<< record.SequenceId << ',' << record.FrameIndex << ',' << record.DomainId << ','
				<< CoverageText(record.CoverageFlags) << '\n';
		}
	}
	
	std::vector<FDW_SD::Split::NSSplitCatalogRecord> parsedCatalog;
	
	if (!FDW_SD::Split::DatasetSplit::LoadCatalog(catalogPath, parsedCatalog).Succeeded()) return 5;
	
	auto sortedCatalog = catalog;
	std::sort(sortedCatalog.begin(), sortedCatalog.end(), [](const auto& left, const auto& right) {
		return left.SampleId < right.SampleId;
	});
	
	if (parsedCatalog != sortedCatalog) return 5;
	
	FDW_SD::Split::NSSplitConfig config{};
	config.DatasetId = datasetId;
	config.SplitSeed = splitSeed;
	config.ValidationBasisPoints = 5'000u;
	
	std::vector<FDW_SD::Split::NSSplitManifestEntry> firstEntries;
	
	FDW_SD::Split::NSSplitReport firstReport{};
	auto result = FDW_SD::Split::DatasetSplit::Build(dataset, config, catalog, firstEntries, firstReport);
	
	if (!result.Succeeded() || firstEntries.size() != catalog.size()) {
		std::cerr << result.Message << '\n';
		return 6;
	}
	
	for (const auto& entry : firstEntries) {
	
		if (entry.LeakageGroupId == trainGroup && entry.Split != static_cast<std::uint32_t>(FDW_SD::Split::NSSplit::Train)) return 7;
	
		if (entry.DomainId == 2u && entry.Split != static_cast<std::uint32_t>(FDW_SD::Split::NSSplit::UnseenTest)) return 8;
	
		if (entry.DomainId == 3u && entry.Split != static_cast<std::uint32_t>(FDW_SD::Split::NSSplit::Stress)) return 9;
	}
	
	for (const auto& stats : firstReport.Visibility) {
		if (stats.ValidValueCount == 0u || stats.FullyLitValueCount == 0u ||
			stats.FullyOccludedValueCount == 0u || stats.PenumbraValueCount == 0u ||
			stats.FullyLitSamplingWeight <= 0.0f || stats.FullyOccludedSamplingWeight <= 0.0f ||
			stats.PenumbraSamplingWeight <= 0.0f) 
		{
			return 10;
		}
	}

	const auto firstManifest = temp / "out-a/splits.nssplit";
	
	if (!FDW_SD::Split::DatasetSplit::WriteManifest(firstManifest, firstReport, firstEntries).Succeeded() ||
		!FDW_SD::Split::DatasetSplit::WriteReportCSV(temp / "out-a/coverage.csv", firstReport).Succeeded()) 
	{
		return 11;
	}
	
	FDW_SD::Split::NSLoadedSplitManifest loaded{};
	
	if (!FDW_SD::Split::DatasetSplit::LoadManifest(firstManifest, loaded).Succeeded() || loaded.Entries != firstEntries) return 12;

	const auto relocated = temp / "relocated";
	std::filesystem::copy(dataset, relocated, std::filesystem::copy_options::recursive, error);
	
	if (error) return 13;
	
	std::reverse(catalog.begin(), catalog.end());
	std::vector<FDW_SD::Split::NSSplitManifestEntry> secondEntries;
	
	FDW_SD::Split::NSSplitReport secondReport{};
	result = FDW_SD::Split::DatasetSplit::Build(relocated, config, catalog, secondEntries, secondReport);
	const auto secondManifest = temp / "different/location/splits.nssplit";
	
	if (!result.Succeeded() || !FDW_SD::Split::DatasetSplit::WriteManifest(secondManifest, secondReport, secondEntries).Succeeded()) return 14;
	
	std::vector<std::byte> firstBytes;
	std::vector<std::byte> secondBytes;
	if (!ReadBytes(firstManifest, firstBytes) || !ReadBytes(secondManifest, secondBytes) || firstBytes != secondBytes) return 15;

	auto badSequence = catalog;
	for (auto& record : badSequence) {
		if (record.SequenceId == 5001u && record.FrameIndex == 11u) record.LeakageGroupId = validationGroup;
	}
	
	if (FDW_SD::Split::DatasetSplit::Build(dataset, config, badSequence, secondEntries, secondReport).Succeeded()) return 16;
	
	auto badCoverage = catalog;
	
	for (auto& record : badCoverage) {
		record.CoverageFlags &= ~static_cast<std::uint32_t>(FDW_SD::Split::NSCoverageFlags::LargePenumbra);
	}
	
	if (FDW_SD::Split::DatasetSplit::Build(dataset, config, badCoverage, secondEntries, secondReport).Succeeded()) return 17;

	std::fstream corrupt(firstManifest, std::ios::binary | std::ios::in | std::ios::out);
	
	corrupt.seekg(-1, std::ios::end);
	
	char byte = 0;
	
	corrupt.read(&byte, 1);
	
	byte ^= 0x5a;
	corrupt.seekp(-1, std::ios::end);
	corrupt.write(&byte, 1);
	corrupt.close();
	
	if (FDW_SD::Split::DatasetSplit::LoadManifest(firstManifest, loaded).Succeeded()) return 18;
	
	std::filesystem::remove_all(temp, error);
	return 0;
}

int main() {
	return DatasetSplitTest::Run();
}
