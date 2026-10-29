#pragma once

#include <ShadowData/DatasetFormat.h>

#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace NSNeuralShadowDataset {

inline constexpr std::uint32_t SAMPLE_FILE_MAGIC = 0x3153'534eu; // NSS1
inline constexpr std::uint32_t MANIFEST_FILE_MAGIC = 0x314d'534eu; // NSM1
inline constexpr std::uint32_t SAMPLE_FILE_VERSION = 1u;
inline constexpr std::uint32_t MANIFEST_FILE_VERSION = 1u;
inline constexpr std::uint32_t COMPLETION_MARKER = 0x454e'4f44u; // DONE
inline constexpr std::uint32_t MAX_SAMPLE_BLOCK_COUNT = 32u;
inline constexpr std::uint32_t MAX_MANIFEST_ENTRY_COUNT = 1'000'000u;
inline constexpr std::uint32_t MAX_RELATIVE_PATH_BYTES = 1'024u;
inline constexpr std::uint64_t MAX_SAMPLE_FILE_BYTES = 2ull * 1024ull * 1024ull * 1024ull;
inline constexpr std::uint64_t MAX_MANIFEST_FILE_BYTES = 64ull * 1024ull * 1024ull;
inline constexpr const char* MANIFEST_FILENAME = "manifest.nsmanifest";

enum class NSSampleBlockKind : std::uint32_t {
	WorkHeader = 1u,
	WorkItems = 2u,
	PixelFeatures = 3u,
	LightFeatures = 4u,
	HardInputVisibility = 5u,
	RTTargetVisibility = 6u,
	TargetValidity = 7u,
	OutputHeader = 8u,
	CaptureMetadata = 10u,
	RTTargetMetadata = 11u,
	NeuralOutputHeader = 12u,
	NeuralOutputWorkItems = 13u,
	NeuralOutputLowHard = 14u,
	NeuralOutputLowBlocker = 15u,
	NeuralOutputLowValidity = 16u,
	NeuralOutputReceiverFeatures = 17u,
	NeuralOutputLightFeatures = 18u,
	NeuralOutputTargetVisibility = 19u,
	NeuralOutputTargetValidity = 20u
};

enum class NSDatasetErrorCode : std::uint32_t {
	None = 0u,
	InvalidArgument,
	UnsafeRelativePath,
	IoFailure,
	FileTooLarge,
	Truncated,
	IdentityMismatch,
	VersionMismatch,
	LayoutMismatch,
	FormatMismatch,
	Incomplete,
	ChecksumMismatch,
	BoundsViolation,
	OverlappingBlocks,
	DuplicateBlock,
	ManifestConflict,
	SampleMismatch
};

enum class NSRegenerationStatus : std::uint32_t {
	GenerateMissing = 0u,
	SkipValid = 1u,
	RegenerateInvalid = 2u,
	Conflict = 3u
};

struct NSSampleFileHeader {
	std::uint32_t Magic = SAMPLE_FILE_MAGIC;
	std::uint32_t Version = SAMPLE_FILE_VERSION;
	std::uint32_t HeaderByteSize = 0u;
	std::uint32_t BlockRecordByteSize = 0u;
	std::uint64_t FileByteSize = 0u;
	std::uint64_t SampleId = 0u;
	std::uint64_t GenerationId = 0u;
	std::uint32_t BlockCount = 0u;
	std::uint32_t Padding0 = 0u;
	std::uint32_t LayoutOffset = 0u;
	std::uint32_t BlockTableOffset = 0u;
	std::uint64_t PayloadOffset = 0u;
	std::uint32_t HeaderChecksum = 0u;
	std::uint32_t ContentChecksum = 0u;
	std::uint32_t CompletionMarker = 0u;
	std::uint32_t Flags = 0u;
	FDW_SD::DatasetFormat::NSDatasetLayoutMetadata Layout{};
};

struct NSSampleBlockRecord {
	std::uint32_t Kind = 0u;
	std::uint32_t Version = 0u;
	std::uint64_t Offset = 0u;
	std::uint64_t ByteSize = 0u;
	std::uint64_t ElementCount = 0u;
	std::uint32_t Checksum = 0u;
	std::uint32_t Flags = 0u;
};

struct NSDatasetManifestHeader {
	std::uint32_t Magic = MANIFEST_FILE_MAGIC;
	std::uint32_t Version = MANIFEST_FILE_VERSION;
	std::uint32_t HeaderByteSize = 0u;
	std::uint32_t EntryRecordByteSize = 0u;
	std::uint64_t FileByteSize = 0u;
	std::uint64_t DatasetId = 0u;
	std::uint32_t EntryCount = 0u;
	std::uint32_t Flags = 0u;
	std::uint64_t EntriesOffset = 0u;
	std::uint64_t PathsOffset = 0u;
	std::uint32_t HeaderChecksum = 0u;
	std::uint32_t ContentChecksum = 0u;
	std::uint32_t CompletionMarker = 0u;
	std::uint32_t Padding0 = 0u;
	FDW_SD::DatasetFormat::NSDatasetLayoutMetadata Layout{};
};

struct NSDatasetManifestEntryRecord {
	std::uint64_t SampleId = 0u;
	std::uint64_t GenerationId = 0u;
	std::uint64_t SampleByteSize = 0u;
	std::uint64_t PathOffset = 0u;
	std::uint32_t PathByteSize = 0u;
	std::uint32_t SampleChecksum = 0u;
	std::uint32_t Flags = 0u;
	std::uint32_t Padding0 = 0u;
};

struct NSDatasetResult {
	NSDatasetErrorCode Code = NSDatasetErrorCode::None;
	std::string Message;
	
	constexpr bool Succeeded() const { return Code == NSDatasetErrorCode::None; }
};

struct NSSampleBlockInput {
	NSSampleBlockKind Kind = NSSampleBlockKind::WorkHeader;
	std::uint32_t Version = 1u;
	std::uint64_t ElementCount = 0u;
	std::span<const std::byte> Bytes;
};

struct NSLoadedSampleBlock {
	NSSampleBlockRecord Record{};
	std::vector<std::byte> Bytes;
};

struct NSLoadedSample {
	NSSampleFileHeader Header{};
	std::vector<NSLoadedSampleBlock> Blocks;
};

struct NSDatasetManifestEntry {
	std::uint64_t SampleId = 0u;
	std::uint64_t GenerationId = 0u;
	std::uint64_t SampleByteSize = 0u;
	std::uint32_t SampleChecksum = 0u;
	std::filesystem::path RelativePath;
};

struct NSLoadedDatasetManifest {
	NSDatasetManifestHeader Header{};
	std::vector<NSDatasetManifestEntry> Entries;
};

class DatasetContainer {

public:
	static std::uint32_t ComputeCRC32(std::span<const std::byte> bytes);
	static bool IsSafeDatasetRelativePath(const std::filesystem::path& relativePath);
	static NSDatasetResult LoadSampleFile(const std::filesystem::path& datasetRoot,const std::filesystem::path& relativePath,NSLoadedSample& sample);
	static NSDatasetResult LoadDatasetManifest(const std::filesystem::path& datasetRoot,NSLoadedDatasetManifest& manifest);

public:
	static NSDatasetResult PublishDatasetSample(const std::filesystem::path& datasetRoot,std::uint64_t datasetId,
												const std::filesystem::path& relativePath,std::uint64_t sampleId,
												std::uint64_t generationId,std::span<const NSSampleBlockInput> blocks);
    
	static NSRegenerationStatus GetSampleRegenerationStatus(const std::filesystem::path &datasetRoot, std::uint64_t datasetId,
                                                            const std::filesystem::path &relativePath, std::uint64_t sampleId,
                                                            std::uint64_t generationId, NSDatasetResult &detail);

  protected:
    static NSDatasetResult Failure(NSDatasetErrorCode code, std::string message);
    static bool CheckedAdd(std::uint64_t a, std::uint64_t b, std::uint64_t &result);
    static bool CheckedMultiply(std::uint64_t a, std::uint64_t b, std::uint64_t &result);
    static bool AlignUp16(std::uint64_t value, std::uint64_t &result);
    static bool PathToUtf8(const std::filesystem::path &path, std::string &value);
    static bool Utf8ToPath(std::string_view value, std::filesystem::path &path);

protected:
	static NSDatasetResult ReadFileLimited(const std::filesystem::path &path, std::uint64_t maximumBytes, std::vector<std::byte> &bytes);
    static NSDatasetResult AtomicWriteFile(const std::filesystem::path &target, std::span<const std::byte> bytes);

protected:
	template<class T>
	static std::uint32_t ComputeHeaderChecksum(T header);

protected:
	static NSDatasetResult ValidateStoredDatasetLayout(const FDW_SD::DatasetFormat::NSDatasetLayoutMetadata& layout);
	
	static NSDatasetResult BuildSampleBytes(std::uint64_t sampleId,std::uint64_t generationId,std::span<const NSSampleBlockInput> blocks,std::vector<std::byte>& bytes);
	static NSDatasetResult BuildManifestBytes(std::uint64_t datasetId,std::span<const NSDatasetManifestEntry> inputEntries,std::vector<std::byte>& bytes);
};

}
