#include <Dataset/DatasetContainer.h>
#include <D3DFramework/GraphicUtilites/BufferReadbackBatch.h>

class DatasetContainerTest {
public:
	static int Run();

protected:
	struct TemporaryDirectory {
		std::filesystem::path Path;
		~TemporaryDirectory() {
			std::error_code error;
			if (Path.filename().wstring().starts_with(L"fdw_dataset_test_")) {
				std::filesystem::remove_all(Path, error);
			}
		}
	};

	static bool FlipByte(const std::filesystem::path& path, std::uint64_t offset) {
		std::fstream stream(path, std::ios::binary | std::ios::in | std::ios::out);
		
		if (!stream) return false;
		
		stream.seekg(static_cast<std::streamoff>(offset));
		char value = 0;
		stream.read(&value, 1);
		
		if (!stream) return false;
		
		value ^= 0x5a;
		stream.seekp(static_cast<std::streamoff>(offset));
		stream.write(&value, 1);
		return static_cast<bool>(stream);
	}
};

int DatasetContainerTest::Run() {
	const std::array crcFixture{
		std::byte{ '1' }, std::byte{ '2' }, std::byte{ '3' },
		std::byte{ '4' }, std::byte{ '5' }, std::byte{ '6' },
		std::byte{ '7' }, std::byte{ '8' }, std::byte{ '9' }
	};

	if (NSNeuralShadowDataset::DatasetContainer::ComputeCRC32(crcFixture) != 0xcbf4'3926u) {
		std::cerr << "CRC32 reference changed.\n";
		return 1;
	}

	if (NSNeuralShadowDataset::DatasetContainer::IsSafeDatasetRelativePath("../escape.nssample") ||
		NSNeuralShadowDataset::DatasetContainer::IsSafeDatasetRelativePath("C:/absolute.nssample") ||
		!NSNeuralShadowDataset::DatasetContainer::IsSafeDatasetRelativePath("samples/scene_a/sample_42.nssample")) {
		std::cerr << "Relative dataset path policy changed.\n";
		return 2;
	}

	const std::uint64_t readbackSizes[] = { 1u, 256u, 257u };
	std::vector<FD3DW::BufferReadbackLayoutEntry> readbackLayout;
	std::uint64_t readbackBytes = 0u;
	if (!FD3DW::BuildBufferReadbackLayout(readbackSizes, readbackLayout, readbackBytes) ||
		readbackLayout.size() != 3u || readbackLayout[0].ReadbackOffset != 0u ||
		readbackLayout[1].ReadbackOffset != 256u || readbackLayout[2].ReadbackOffset != 512u ||
		readbackBytes != 769u) {
		std::cerr << "Readback layout changed.\n";
		return 3;
	}

	const auto unique = std::to_wstring(GetCurrentProcessId()) + L"_" +std::to_wstring(std::chrono::steady_clock::now().time_since_epoch().count());
	
	TemporaryDirectory temporary{
		std::filesystem::temp_directory_path() / (L"fdw_dataset_test_" + unique)
	};

	auto datasetRoot = temporary.Path / "root_a";
	const std::filesystem::path firstRelative = "samples/scene_a/sample_42.nssample";
	const std::filesystem::path secondRelative = "samples/scene_b/sample_43.nssample";
	constexpr std::uint64_t datasetId = 0x1122'3344'5566'7788ull;

	const std::array firstMetadata{
		std::byte{ 1 }, std::byte{ 3 }, std::byte{ 5 }, std::byte{ 7 }, std::byte{ 9 }
	};

	const std::array firstPayload{
		std::byte{ 2 }, std::byte{ 4 }, std::byte{ 6 }, std::byte{ 8 },
		std::byte{ 10 }, std::byte{ 12 }, std::byte{ 14 }, std::byte{ 16 }, std::byte{ 18 }
	};

	const NSNeuralShadowDataset::NSSampleBlockInput firstBlocks[] = {
		{ NSNeuralShadowDataset::NSSampleBlockKind::WorkHeader, 1u, 1u, firstMetadata },
		{ NSNeuralShadowDataset::NSSampleBlockKind::RTTargetVisibility, 1u, firstPayload.size(), firstPayload }
	};

	auto result = NSNeuralShadowDataset::DatasetContainer::PublishDatasetSample(datasetRoot, datasetId, firstRelative, 42u, 9u, firstBlocks);
	
	if (!result.Succeeded()) {
		std::cerr << "Initial publish failed: " << result.Message << '\n';
		return 4;
	}

	NSNeuralShadowDataset::NSLoadedSample loadedSample{};
	result = NSNeuralShadowDataset::DatasetContainer::LoadSampleFile(datasetRoot, firstRelative, loadedSample);
	if (!result.Succeeded() || loadedSample.Header.SampleId != 42u ||
		loadedSample.Header.GenerationId != 9u || loadedSample.Blocks.size() != 2u ||
		loadedSample.Blocks[0].Bytes != std::vector<std::byte>(firstMetadata.begin(), firstMetadata.end()) ||
		loadedSample.Blocks[1].Bytes != std::vector<std::byte>(firstPayload.begin(), firstPayload.end())) {
	
		std::cerr << "Sample byte/metadata roundtrip failed.\n";
		return 5;
	}
	NSNeuralShadowDataset::NSLoadedDatasetManifest manifest{};
	result = NSNeuralShadowDataset::DatasetContainer::LoadDatasetManifest(datasetRoot, manifest);
	if (!result.Succeeded() || manifest.Header.DatasetId != datasetId ||
		manifest.Entries.size() != 1u || manifest.Entries[0].RelativePath != firstRelative) {
		
		std::cerr << "Manifest roundtrip failed.\n";
		return 6;
	}
	NSNeuralShadowDataset::NSDatasetResult regenerationDetail{};
	if (NSNeuralShadowDataset::DatasetContainer::GetSampleRegenerationStatus(
		datasetRoot, datasetId, firstRelative, 42u, 9u, regenerationDetail) !=
		NSNeuralShadowDataset::NSRegenerationStatus::SkipValid) {
		
		std::cerr << "Valid sample was not skipped.\n";
		return 7;
	}

	const std::array secondPayload{ std::byte{ 21 }, std::byte{ 22 }, std::byte{ 23 } };
	const NSNeuralShadowDataset::NSSampleBlockInput secondBlocks[] = {{ NSNeuralShadowDataset::NSSampleBlockKind::TargetValidity, 1u, secondPayload.size(), secondPayload }};
	result = NSNeuralShadowDataset::DatasetContainer::PublishDatasetSample(datasetRoot, datasetId, secondRelative, 43u, 10u, secondBlocks);
	
	if (!result.Succeeded()) return 8;
	
	result = NSNeuralShadowDataset::DatasetContainer::LoadDatasetManifest(datasetRoot, manifest);
	
	if (!result.Succeeded() || manifest.Entries.size() != 2u ||
		NSNeuralShadowDataset::DatasetContainer::GetSampleRegenerationStatus(
			datasetRoot, datasetId, firstRelative, 42u, 9u, regenerationDetail) !=
			NSNeuralShadowDataset::NSRegenerationStatus::SkipValid) {
		
		std::cerr << "Partial generation changed an existing sample.\n";
		return 9;
	}
	
	if (NSNeuralShadowDataset::DatasetContainer::GetSampleRegenerationStatus(datasetRoot,datasetId,"samples/scene_c/sample_44.nssample",44u,11u, regenerationDetail) != NSNeuralShadowDataset::NSRegenerationStatus::GenerateMissing ||
		NSNeuralShadowDataset::DatasetContainer::GetSampleRegenerationStatus(datasetRoot, datasetId, firstRelative, 42u, 99u, regenerationDetail) != NSNeuralShadowDataset::NSRegenerationStatus::RegenerateInvalid) {
		
		std::cerr << "Missing/changed-generation decisions failed.\n";
		return 10;
	}

	const auto relocatedRoot = temporary.Path / "relocated_root";
	std::error_code filesystemError;
	std::filesystem::rename(datasetRoot, relocatedRoot, filesystemError);
	
	if (filesystemError) return 11;
	
	datasetRoot = relocatedRoot;
	if (NSNeuralShadowDataset::DatasetContainer::GetSampleRegenerationStatus(
		datasetRoot, datasetId, firstRelative, 42u, 9u, regenerationDetail) !=
		NSNeuralShadowDataset::NSRegenerationStatus::SkipValid) {
		
		std::cerr << "Relative-root relocation failed.\n";
		return 12;
	}

	result = NSNeuralShadowDataset::DatasetContainer::LoadSampleFile(datasetRoot, firstRelative, loadedSample);
	
	if (!result.Succeeded() || loadedSample.Blocks.empty()) return 13;
	
	if (!FlipByte(datasetRoot / firstRelative, loadedSample.Blocks.back().Record.Offset)) return 14;
	
	result = NSNeuralShadowDataset::DatasetContainer::LoadSampleFile(datasetRoot, firstRelative, loadedSample);
	
	if (result.Code != NSNeuralShadowDataset::NSDatasetErrorCode::ChecksumMismatch ||
		NSNeuralShadowDataset::DatasetContainer::GetSampleRegenerationStatus(
			datasetRoot, datasetId, firstRelative, 42u, 9u, regenerationDetail) !=
			NSNeuralShadowDataset::NSRegenerationStatus::RegenerateInvalid) {
		
		std::cerr << "Payload corruption was not detected/regenerated.\n";
		return 15;
	}
	result = NSNeuralShadowDataset::DatasetContainer::PublishDatasetSample( datasetRoot, datasetId, firstRelative, 42u, 9u, firstBlocks);
	
	if (!result.Succeeded() || NSNeuralShadowDataset::DatasetContainer::GetSampleRegenerationStatus(
		datasetRoot, datasetId, firstRelative, 42u, 9u, regenerationDetail) !=
		NSNeuralShadowDataset::NSRegenerationStatus::SkipValid)
	{
		return 16;
	}

	const auto secondPath = datasetRoot / secondRelative;
	const auto secondSize = std::filesystem::file_size(secondPath, filesystemError);
	
	if (filesystemError || secondSize == 0u) return 17;
	
	std::filesystem::resize_file(secondPath, secondSize - 1u, filesystemError);
	if (filesystemError) return 18;
	
	result = NSNeuralShadowDataset::DatasetContainer::LoadSampleFile(datasetRoot, secondRelative, loadedSample);
	
	if (result.Code != NSNeuralShadowDataset::NSDatasetErrorCode::Truncated ||
		NSNeuralShadowDataset::DatasetContainer::GetSampleRegenerationStatus(
			datasetRoot, datasetId, secondRelative, 43u, 10u, regenerationDetail) !=
			NSNeuralShadowDataset::NSRegenerationStatus::RegenerateInvalid) {
		
		std::cerr << "Sample truncation was not detected/regenerated.\n";
		return 19;
	}
	result = NSNeuralShadowDataset::DatasetContainer::PublishDatasetSample( datasetRoot, datasetId, secondRelative, 43u, 10u, secondBlocks);
	if (!result.Succeeded()) return 20;

	result = NSNeuralShadowDataset::DatasetContainer::PublishDatasetSample( datasetRoot, datasetId, firstRelative, 99u, 9u, firstBlocks);
	if (result.Code != NSNeuralShadowDataset::NSDatasetErrorCode::ManifestConflict) {
		std::cerr << "Manifest identity conflict was not rejected.\n";
		return 21;
	}

	if (!FlipByte(datasetRoot / NSNeuralShadowDataset::MANIFEST_FILENAME, sizeof(NSNeuralShadowDataset::NSDatasetManifestHeader))) return 22;
	
	result = NSNeuralShadowDataset::DatasetContainer::LoadDatasetManifest(datasetRoot, manifest);
	if (result.Code != NSNeuralShadowDataset::NSDatasetErrorCode::ChecksumMismatch) {
		std::cerr << "Manifest corruption was not detected.\n";
		return 23;
	}

	return 0;
}

int main() {
	return DatasetContainerTest::Run();
}
