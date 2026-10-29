#pragma once

#include <Dataset/DatasetContainer.h>
#include <ShadowData/CaptureConfiguration.h>

namespace FDW_SD::Split {

inline constexpr std::uint32_t SPLIT_MANIFEST_MAGIC = 0x3150'534eu; // NSP1
inline constexpr std::uint32_t SPLIT_MANIFEST_VERSION = 1u;
inline constexpr std::uint32_t SPLIT_POLICY_VERSION = 1u;
inline constexpr std::uint32_t SPLIT_COMPLETION_MARKER = 0x454e'4f44u; // DONE
inline constexpr std::uint32_t SPLIT_MAX_ENTRY_COUNT = 1'000'000u;
inline constexpr std::uint64_t SPLIT_MAX_FILE_BYTES = 128ull * 1024ull * 1024ull;
inline constexpr float SPLIT_MAX_VISIBILITY_CLASS_WEIGHT = 8.0f;

enum class NSSplit : std::uint32_t {
	Train = 0u,
	Validation = 1u,
	UnseenTest = 2u,
	Stress = 3u,
	Count = 4u
};

enum class NSCoverageFlags : std::uint32_t {
	None = 0u,
	ContactShadow = 1u << 0u,
	ThinGeometry = 1u << 1u,
	LargePenumbra = 1u << 2u,
	Motion = 1u << 3u,
	OutOfDistribution = 1u << 4u,
	StressCase = 1u << 5u
};

inline constexpr std::uint32_t REQUIRED_COVERAGE_FLAGS =
	static_cast<std::uint32_t>(NSCoverageFlags::ContactShadow) |
	static_cast<std::uint32_t>(NSCoverageFlags::ThinGeometry) |
	static_cast<std::uint32_t>(NSCoverageFlags::LargePenumbra) |
	static_cast<std::uint32_t>(NSCoverageFlags::Motion) |
	static_cast<std::uint32_t>(NSCoverageFlags::OutOfDistribution);

struct NSSplitCatalogRecord {
	std::uint64_t SampleId = 0u;
	std::uint64_t GenerationId = 0u;
	std::uint64_t LeakageGroupId = 0u;
	std::uint64_t SequenceId = 0u;
	std::uint64_t DomainId = 0u;
	
	std::uint32_t FrameIndex = 0u;
	std::uint32_t CoverageFlags = 0u;
	
	constexpr bool operator==(const NSSplitCatalogRecord&) const = default;
};

struct NSSplitConfig {
	std::uint64_t DatasetId = 0u;
	std::uint64_t SplitSeed = 1u;
	
	std::uint32_t ValidationBasisPoints = 1'000u;
	std::uint32_t RequiredCoverageFlags = REQUIRED_COVERAGE_FLAGS;
};

struct NSSplitManifestHeader {
	std::uint32_t Magic = SPLIT_MANIFEST_MAGIC;
	std::uint32_t Version = SPLIT_MANIFEST_VERSION;
	std::uint32_t HeaderByteSize = 0u;
	std::uint32_t EntryRecordByteSize = 0u;
	
	std::uint64_t FileByteSize = 0u;
	std::uint64_t DatasetId = 0u;
	std::uint64_t SplitSeed = 0u;
	
	std::uint32_t EntryCount = 0u;
	std::uint32_t EntriesOffset = 0u;
	std::uint32_t PolicyVersion = SPLIT_POLICY_VERSION;
	std::uint32_t ValidationBasisPoints = 0u;
	std::uint32_t RequiredCoverageFlags = REQUIRED_COVERAGE_FLAGS;
	std::uint32_t CompletionMarker = 0u;
	std::uint32_t HeaderChecksum = 0u;
	std::uint32_t ContentChecksum = 0u;
	std::uint32_t SplitCounts[4]{};
	std::uint32_t LeakageGroupCount = 0u;
	std::uint32_t DomainCount = 0u;
	std::uint32_t Padding[8]{};
};

struct NSSplitManifestEntry {
	std::uint64_t SampleId = 0u;
	std::uint64_t GenerationId = 0u;
	std::uint64_t LeakageGroupId = 0u;
	std::uint64_t SequenceId = 0u;
	std::uint64_t DomainId = 0u;
	
	std::uint32_t FrameIndex = 0u;
	std::uint32_t Split = 0u;
	std::uint32_t CoverageFlags = 0u;
	std::uint32_t Padding[3]{};
	
	constexpr bool operator==(const NSSplitManifestEntry&) const = default;
};

struct NSSplitVisibilityStats {
	std::uint64_t ValidValueCount = 0u;
	std::uint64_t FullyLitValueCount = 0u;
	std::uint64_t FullyOccludedValueCount = 0u;
	std::uint64_t PenumbraValueCount = 0u;
	std::uint64_t InvalidValueCount = 0u;
	
	float FullyLitSamplingWeight = 0.0f;
	float FullyOccludedSamplingWeight = 0.0f;
	float PenumbraSamplingWeight = 0.0f;
};

struct NSSplitReport {
	NSSplitManifestHeader Header{};
	NSSplitVisibilityStats Visibility[4]{};
	
	std::uint32_t CoverageSampleCounts[4][6]{};
	std::uint32_t LeakageGroupCounts[4]{};
};

struct NSLoadedSplitManifest {
	NSSplitManifestHeader Header{};
	
	std::vector<NSSplitManifestEntry> Entries;
};

struct NSSplitResult {
	NSNeuralShadowDataset::NSDatasetErrorCode Code =  NSNeuralShadowDataset::NSDatasetErrorCode::None;
	
	std::string Message;
	bool Succeeded() const;
};

class DatasetSplit {
public:
	static NSSplitResult LoadCatalog(const std::filesystem::path& path,std::vector<NSSplitCatalogRecord>& records);

	static NSSplitResult Build(const std::filesystem::path& datasetRoot,const NSSplitConfig& config,std::span<const NSSplitCatalogRecord> catalog,std::vector<NSSplitManifestEntry>& entries, NSSplitReport& report);

public:
	static NSSplitResult WriteManifest(const std::filesystem::path& path,const NSSplitReport& report,std::span<const NSSplitManifestEntry> entries);
	static NSSplitResult LoadManifest(const std::filesystem::path& path,NSLoadedSplitManifest& manifest);

public:
	static NSSplitResult WriteReportCSV(const std::filesystem::path& path,const NSSplitReport& report);

public:
	static const char* Name(NSSplit split);

protected:
	inline static constexpr std::string_view CATALOG_HEADER = "sample_id,generation_id,leakage_group_id,sequence_id,frame_index,domain_id,coverage_flags";
	inline static constexpr std::uint32_t KNOWN_COVERAGE_FLAGS = REQUIRED_COVERAGE_FLAGS | static_cast<std::uint32_t>(NSCoverageFlags::StressCase);
	
protected:
	static NSSplitResult Failure(NSNeuralShadowDataset::NSDatasetErrorCode code, std::string message);
	

protected:
	template<class T>
	static bool ParseUnsigned(std::string_view text, T& value);
	

protected:
	static std::vector<std::string_view> Split(std::string_view text, char separator);
	
	static bool ParseCoverage(std::string_view text, std::uint32_t& flags);
	static bool HasFlag(std::uint32_t flags, NSCoverageFlags flag);
	static std::uint64_t HashGroup(std::uint64_t seed, std::uint64_t group);
	
	static const NSNeuralShadowDataset::NSLoadedSampleBlock* FindBlock(const NSNeuralShadowDataset::NSLoadedSample& sample,NSNeuralShadowDataset::NSSampleBlockKind kind);
	
protected:
	template<class T>
	static bool ReadExact(std::span<const std::byte> bytes, T& value);
	
	static std::uint16_t ReadHalf(std::span<const std::byte> bytes, std::uint64_t index);
	
protected:
	static std::string CaptureScene(const NSNeuralShadowDataset::NSLoadedSampleBlock& block,FDW_SD::Capture::NSCaptureMetadataHeader& header);
	
	static float SamplingWeight(std::uint64_t valid, std::uint64_t count);
	
	static void FinalizeWeights(NSSplitVisibilityStats& stats);
	static void AccumulateStats(NSSplitVisibilityStats& destination, const NSSplitVisibilityStats& source);
	
	static std::uint32_t HeaderChecksum(NSSplitManifestHeader header);

protected:
	static NSSplitResult AtomicWrite(const std::filesystem::path& path,std::span<const std::byte> bytes);
	static NSSplitResult ReadFile(const std::filesystem::path& path,std::vector<std::byte>& bytes);
};

}
