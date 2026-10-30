#include "pch.h"

#include <ShadowData/DatasetSplit.h>

#include <ShadowData/CaptureConfiguration.h>
#include <ShadowData/NeuralShadowData.h>
#include <ShadowData/GroundTruthMetadata.h>


namespace FDW_SD::Split {

bool NSSplitResult::Succeeded() const {
	return Code == NSNeuralShadowDataset::NSDatasetErrorCode::None;
}

NSSplitResult DatasetSplit::Failure(NSNeuralShadowDataset::NSDatasetErrorCode code, std::string message) {
	return { code, std::move(message) };
}

template<class T>
bool DatasetSplit::ParseUnsigned(std::string_view text, T& value) {
	if (text.empty()) return false;
	
	const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
	return result.ec == std::errc{} && result.ptr == text.data() + text.size();
}

std::vector<std::string_view> DatasetSplit::Split(std::string_view text, char separator) {
	std::vector<std::string_view> fields;
	std::size_t start = 0u;
	
	for (;;) {
		const auto end = text.find(separator, start);
		fields.push_back(text.substr(start, end == std::string_view::npos ? end : end - start));
		
		if (end == std::string_view::npos) return fields;
		start = end + 1u;
	}
}

bool DatasetSplit::ParseCoverage(std::string_view text, std::uint32_t& flags) {
	flags = 0u;
	
	if (text == "none") return true;
	
	for (const auto token : Split(text, '|')) {
		
		if (token == "contact") flags |= static_cast<std::uint32_t>(NSCoverageFlags::ContactShadow);
		else if (token == "thin") flags |= static_cast<std::uint32_t>(NSCoverageFlags::ThinGeometry);
		else if (token == "large_penumbra") flags |= static_cast<std::uint32_t>(NSCoverageFlags::LargePenumbra);
		else if (token == "motion") flags |= static_cast<std::uint32_t>(NSCoverageFlags::Motion);
		else if (token == "ood") flags |= static_cast<std::uint32_t>(NSCoverageFlags::OutOfDistribution);
		else if (token == "stress") flags |= static_cast<std::uint32_t>(NSCoverageFlags::StressCase);
		else return false;
	
	}
	
	return flags != 0u;
}

bool DatasetSplit::HasFlag(std::uint32_t flags, NSCoverageFlags flag) {
	return (flags & static_cast<std::uint32_t>(flag)) != 0u;
}

std::uint64_t DatasetSplit::HashGroup(std::uint64_t seed, std::uint64_t group) {
	std::uint64_t hash = 14695981039346656037ull;
	for (unsigned shift = 0u; shift < 64u; shift += 8u) {
		hash = (hash ^ static_cast<std::uint8_t>(seed >> shift)) * 1099511628211ull;
	}
	
	for (unsigned shift = 0u; shift < 64u; shift += 8u) {
		hash = (hash ^ static_cast<std::uint8_t>(group >> shift)) * 1099511628211ull;
	}
	
	return hash;
}

const NSNeuralShadowDataset::NSLoadedSampleBlock* DatasetSplit::FindBlock(const NSNeuralShadowDataset::NSLoadedSample& sample, NSNeuralShadowDataset::NSSampleBlockKind kind) {
	for (const auto& block : sample.Blocks) {
		if (block.Record.Kind == static_cast<std::uint32_t>(kind)) return &block;
	}
	
	return nullptr;
}

template<class T>
bool DatasetSplit::ReadExact(std::span<const std::byte> bytes, T& value) {
	if (bytes.size() != sizeof(T)) return false;
	
	std::memcpy(&value, bytes.data(), sizeof(T));
	return true;
}

std::uint16_t DatasetSplit::ReadHalf(std::span<const std::byte> bytes, std::uint64_t index) {
	std::uint16_t value = 0u;
	std::memcpy(&value, bytes.data() + index * sizeof(value), sizeof(value));
	return value;
}

std::string DatasetSplit::CaptureScene(const NSNeuralShadowDataset::NSLoadedSampleBlock& block, FDW_SD::Capture::NSCaptureMetadataHeader& header) {
	if (!FDW_SD::Capture::CaptureConfiguration::ValidateMetadata(block.Bytes, &header).Succeeded()) return {};
	
	return std::string(reinterpret_cast<const char*>(block.Bytes.data() + header.ScenePathOffset), header.ScenePathByteSize);
}

float DatasetSplit::SamplingWeight(std::uint64_t valid, std::uint64_t count) {
	if (valid == 0u || count == 0u) return 0.0f;
	
	return std::min(SPLIT_MAX_VISIBILITY_CLASS_WEIGHT, static_cast<float>(valid) / (3.0f * static_cast<float>(count)));
}

void DatasetSplit::FinalizeWeights(NSSplitVisibilityStats& stats) {
	stats.FullyLitSamplingWeight = SamplingWeight(stats.ValidValueCount, stats.FullyLitValueCount);
	stats.FullyOccludedSamplingWeight = SamplingWeight(stats.ValidValueCount, stats.FullyOccludedValueCount);
	stats.PenumbraSamplingWeight = SamplingWeight(stats.ValidValueCount, stats.PenumbraValueCount);
}

void DatasetSplit::AccumulateStats(NSSplitVisibilityStats& destination, const NSSplitVisibilityStats& source) {
	destination.ValidValueCount += source.ValidValueCount;
	destination.FullyLitValueCount += source.FullyLitValueCount;
	destination.FullyOccludedValueCount += source.FullyOccludedValueCount;
	destination.PenumbraValueCount += source.PenumbraValueCount;
	destination.InvalidValueCount += source.InvalidValueCount;
}

std::uint32_t DatasetSplit::HeaderChecksum(NSSplitManifestHeader header) {
	header.HeaderChecksum = 0u;
	return NSNeuralShadowDataset::DatasetContainer::ComputeCRC32(std::as_bytes(std::span<const NSSplitManifestHeader>(&header, 1u)));
}

NSSplitResult DatasetSplit::AtomicWrite(const std::filesystem::path& path, std::span<const std::byte> bytes) {
	if (path.empty()) return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::InvalidArgument, "split output path is required");
	
	std::error_code error;
	if (!path.parent_path().empty()) std::filesystem::create_directories(path.parent_path(), error);
	
	if (error) return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::IoFailure, error.message());
	
	static std::atomic<std::uint64_t> nextId{ 1u };
	auto temporary = path;
	temporary += L".tmp." + std::to_wstring(GetCurrentProcessId()) + L"." + std::to_wstring(nextId.fetch_add(1u, std::memory_order_relaxed));
	
	{
		std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
		if (!stream) return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::IoFailure, "failed to create split temporary file");
		
		stream.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
		stream.flush();
		
		if (!stream) {
			std::filesystem::remove(temporary, error);
			return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::IoFailure, "failed while writing split temporary file");
		}
	}
	
	if (!MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
		const auto code = GetLastError();
		std::filesystem::remove(temporary, error);
		return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::IoFailure, "split atomic replace failed: " + std::to_string(code));
	}
	
	return {};
}

NSSplitResult DatasetSplit::ReadFile(const std::filesystem::path& path, std::vector<std::byte>& bytes) {
	std::error_code error;
	const auto size = std::filesystem::file_size(path, error);
	
	if (error) return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::IoFailure, error.message());
	
	if (size < sizeof(NSSplitManifestHeader) || size > SPLIT_MAX_FILE_BYTES) return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::FileTooLarge, "split manifest size is outside supported bounds");
	
	bytes.resize(static_cast<std::size_t>(size));
	std::ifstream stream(path, std::ios::binary);
	
	if (!stream) return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::IoFailure, "failed to open split manifest");
	
	stream.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
	
	if (!stream || static_cast<std::size_t>(stream.gcount()) != bytes.size()) return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::Truncated, "split manifest is truncated");
	
	return {};
}

const char* DatasetSplit::Name(NSSplit split) {
	switch (split) {
	case NSSplit::Train: 
		return "train";
	case NSSplit::Validation:
		return "validation";
	case NSSplit::UnseenTest: 
		return "unseen_test";
	case NSSplit::Stress: 
		return "stress";
	default: 
		return "invalid";
	}
}

NSSplitResult DatasetSplit::LoadCatalog(const std::filesystem::path& path, std::vector<NSSplitCatalogRecord>& records) {
	
	records.clear();
	std::ifstream stream(path);
	
	if (!stream) return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::IoFailure, "failed to open split catalog");
	
	std::string line;
	if (!std::getline(stream, line) || line != CATALOG_HEADER) return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::VersionMismatch, "split catalog header uses an unsupported format");
	
	std::set<std::uint64_t> sampleIds;
	std::uint32_t lineNumber = 1u;
	
	while (std::getline(stream, line)) {
		++lineNumber;
		
		if (line.empty() || line[0] == '#') continue;
		
		const auto fields = Split(line, ',');
		NSSplitCatalogRecord record{};
		if (fields.size() != 7u || !ParseUnsigned(fields[0], record.SampleId) ||
			!ParseUnsigned(fields[1], record.GenerationId) || !ParseUnsigned(fields[2], record.LeakageGroupId) ||
			!ParseUnsigned(fields[3], record.SequenceId) || !ParseUnsigned(fields[4], record.FrameIndex) ||
			!ParseUnsigned(fields[5], record.DomainId) || !ParseCoverage(fields[6], record.CoverageFlags) ||
			record.SampleId == 0u || record.GenerationId == 0u || record.LeakageGroupId == 0u || record.DomainId == 0u ||
			!sampleIds.insert(record.SampleId).second) {
		
			return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::InvalidArgument, "invalid or duplicate split catalog row " + std::to_string(lineNumber));
		}
		
		records.push_back(record);
	}
	if (!stream.eof()) return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::IoFailure, "failed while reading split catalog");
	
	if (records.empty() || records.size() > SPLIT_MAX_ENTRY_COUNT) return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::InvalidArgument, "split catalog must contain bounded samples");
	
	std::sort(records.begin(), records.end(), [](const auto& left, const auto& right) {
		return left.SampleId < right.SampleId;
	});
	
	return {};
}

NSSplitResult DatasetSplit::Build(const std::filesystem::path& datasetRoot,const NSSplitConfig& config,std::span<const NSSplitCatalogRecord> catalog,std::vector<NSSplitManifestEntry>& entries, NSSplitReport& report) {
	
	entries.clear();
	report = {};
	if (datasetRoot.empty() || config.DatasetId == 0u || config.SplitSeed == 0u ||
		config.ValidationBasisPoints == 0u || config.ValidationBasisPoints >= 10'000u || catalog.empty() ||
		config.RequiredCoverageFlags != REQUIRED_COVERAGE_FLAGS) {
		return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::InvalidArgument, "invalid split root/config/catalog");
	}
	
	NSNeuralShadowDataset::NSLoadedDatasetManifest dataset{};
	auto datasetResult = NSNeuralShadowDataset::DatasetContainer::LoadDatasetManifest(datasetRoot, dataset);
	
	if (!datasetResult.Succeeded()) return Failure(datasetResult.Code, datasetResult.Message);
	
	if (dataset.Header.DatasetId != config.DatasetId || dataset.Entries.size() != catalog.size())
		return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::IdentityMismatch, "split catalog does not exactly cover the dataset manifest");

	std::map<std::uint64_t, const NSNeuralShadowDataset::NSDatasetManifestEntry*> datasetById;
	
	for (const auto& item : dataset.Entries) datasetById.emplace(item.SampleId, &item);
	
	std::map<std::uint64_t, std::uint64_t> sequenceGroups;
	std::map<std::uint64_t, std::set<std::uint32_t>> sequenceFrames;
	std::map<std::uint64_t, std::uint64_t> groupDomains;
	std::map<std::string, std::uint64_t> sceneDomains;
	std::map<std::uint64_t, std::uint32_t> groupCoverage;
	std::map<std::uint64_t, std::pair<bool, bool>> groupStress;
	std::map<std::uint64_t, std::pair<bool, bool>> domainOOD;
	std::map<std::uint64_t, std::pair<bool, bool>> domainStress;
	std::map<std::uint64_t, NSSplitVisibilityStats> sampleVisibility;
	std::uint32_t observedCoverage = 0u;

	entries.reserve(catalog.size());
	for (const auto& record : catalog) {
		if (record.SampleId == 0u || record.GenerationId == 0u || record.LeakageGroupId == 0u ||
			record.DomainId == 0u || (record.CoverageFlags & ~KNOWN_COVERAGE_FLAGS) != 0u) {
			return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::InvalidArgument, "split catalog contains invalid identity/coverage");
		}
		
		const auto source = datasetById.find(record.SampleId);
		if (source == datasetById.end() || source->second->GenerationId != record.GenerationId)
			return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::IdentityMismatch, "split catalog sample/generation is absent from dataset");
		
		const auto [groupDomain, insertedDomain] = groupDomains.emplace(record.LeakageGroupId, record.DomainId);
		if (!insertedDomain && groupDomain->second != record.DomainId)
			return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::SampleMismatch, "one leakage group spans multiple domains");
		
		if (record.SequenceId != 0u) {
		
			const auto [sequence, inserted] = sequenceGroups.emplace(record.SequenceId, record.LeakageGroupId);
			if (!inserted && sequence->second != record.LeakageGroupId)
				return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::SampleMismatch, "one sequence spans multiple leakage groups");
		
			if (!sequenceFrames[record.SequenceId].insert(record.FrameIndex).second)
				return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::DuplicateBlock, "sequence frame index is duplicated");
		}
		
		if (HasFlag(record.CoverageFlags, NSCoverageFlags::Motion) && record.SequenceId == 0u)
			return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::SampleMismatch, "motion coverage requires explicit sequence identity");
		
		groupCoverage[record.LeakageGroupId] |= record.CoverageFlags;
		auto& stressState = groupStress[record.LeakageGroupId];
		
		if (HasFlag(record.CoverageFlags, NSCoverageFlags::StressCase)) stressState.first = true;
		else stressState.second = true;
		
		auto& domainState = domainOOD[record.DomainId];
		
		if (HasFlag(record.CoverageFlags, NSCoverageFlags::OutOfDistribution)) domainState.first = true;
		else domainState.second = true;
		
		auto& domainStressState = domainStress[record.DomainId];
		
		if (HasFlag(record.CoverageFlags, NSCoverageFlags::StressCase)) domainStressState.first = true;
		else domainStressState.second = true;
		
		observedCoverage |= record.CoverageFlags;

		NSNeuralShadowDataset::NSLoadedSample sample{};
		datasetResult = NSNeuralShadowDataset::DatasetContainer::LoadSampleFile(datasetRoot, source->second->RelativePath, sample);
		
		if (!datasetResult.Succeeded()) return Failure(datasetResult.Code, datasetResult.Message);
		
		const auto* captureBlock = FindBlock(sample, NSNeuralShadowDataset::NSSampleBlockKind::CaptureMetadata);
		const auto* rtMetadataBlock = FindBlock(sample, NSNeuralShadowDataset::NSSampleBlockKind::RTTargetMetadata);
		const auto* targetBlock = FindBlock(sample, NSNeuralShadowDataset::NSSampleBlockKind::RTTargetVisibility);
		const auto* validityBlock = FindBlock(sample, NSNeuralShadowDataset::NSSampleBlockKind::TargetValidity);
		const auto* outputBlock = FindBlock(sample, NSNeuralShadowDataset::NSSampleBlockKind::OutputHeader);
		
		if (!captureBlock || !rtMetadataBlock || !targetBlock || !validityBlock || !outputBlock)
			return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::Incomplete, "split input sample is not a ground-truth sample");
		
		FDW_SD::Capture::NSCaptureMetadataHeader captureHeader{};
		const auto scene = CaptureScene(*captureBlock, captureHeader);
		FDW_SD::GroundTruth::NSRTTargetMetadataHeader rtHeader{};
		FDW_SD::Output::NSOutputHeader output{};
		
		if (scene.empty() || captureHeader.SampleId != record.SampleId || captureHeader.GenerationId != record.GenerationId ||
			!FDW_SD::GroundTruth::GroundTruthMetadata::Validate(rtMetadataBlock->Bytes, nullptr, nullptr, &rtHeader).Succeeded() ||
			rtHeader.SampleId != record.SampleId || rtHeader.GenerationId != record.GenerationId ||
			!ReadExact(outputBlock->Bytes, output) ||
			output.Producer != static_cast<std::uint32_t>(FDW_SD::Output::NSOutputProducer::RayTracedReference) ||
			output.Completion != static_cast<std::uint32_t>(FDW_SD::Output::NSOutputCompletion::Complete) ||
			output.OutputValueCount != rtHeader.OutputValueCount ||
			targetBlock->Bytes.size() != static_cast<std::uint64_t>(output.OutputValueCount) * sizeof(std::uint16_t) ||
			validityBlock->Bytes.size() != output.OutputValueCount) {


			return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::SampleMismatch, "split input target/capture identity is invalid");
		}
		
		const auto [sceneDomain, insertedScene] = sceneDomains.emplace(scene, record.DomainId);
		
		if (!insertedScene && sceneDomain->second != record.DomainId) return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::SampleMismatch, "one scene identity spans multiple domains");
		
		auto& sampleStats = sampleVisibility[record.SampleId];
		for (std::uint64_t value = 0u; value < validityBlock->Bytes.size(); ++value) {
			
			const auto reason = std::to_integer<std::uint8_t>(validityBlock->Bytes[value]);
			if (reason != static_cast<std::uint8_t>(FDW_SD::Output::NSOutputValidityReason::Valid)) {
				++sampleStats.InvalidValueCount;
				continue;
			}
			
			const auto visibility = FDW_SD::Output::VisibilityEncodingTools::DecodeR16Float(ReadHalf(targetBlock->Bytes, value));
			if (!std::isfinite(visibility) || visibility < 0.0f || visibility > 1.0f) return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::LayoutMismatch, "split target has invalid visibility");
			
			++sampleStats.ValidValueCount;
			
			if (visibility <= 0.05f) ++sampleStats.FullyOccludedValueCount;
			else if (visibility >= 0.95f) ++sampleStats.FullyLitValueCount;
			else ++sampleStats.PenumbraValueCount;
		}

		NSSplitManifestEntry entry{};
		entry.SampleId = record.SampleId;
		entry.GenerationId = record.GenerationId;
		entry.LeakageGroupId = record.LeakageGroupId;
		entry.SequenceId = record.SequenceId;
		entry.DomainId = record.DomainId;
		entry.FrameIndex = record.FrameIndex;
		entry.CoverageFlags = record.CoverageFlags;
		entries.push_back(entry);
	}
	
	for (const auto& [sequence, frames] : sequenceFrames) {
		if (frames.size() < 2u) {
			const auto group = sequenceGroups.at(sequence);
			if (HasFlag(groupCoverage.at(group), NSCoverageFlags::Motion))
				return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::Incomplete, "motion sequence needs at least two distinct frames");
		}
	}
	
	for (const auto& [domain, state] : domainOOD) {
		if (state.first && state.second)
			return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::SampleMismatch, "an OOD domain is mixed with development/stress samples");
	}
	
	for (const auto& [domain, state] : domainStress) {
		if (state.first && state.second)
			return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::SampleMismatch, "a stress domain is mixed with development/unseen samples");
	}
	
	for (const auto& [group, state] : groupStress) {
		if (state.first && state.second)
			return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::SampleMismatch, "a stress leakage group mixes stress and ordinary samples");
	}
	
	if ((observedCoverage & config.RequiredCoverageFlags) != config.RequiredCoverageFlags)
		return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::Incomplete, "dataset does not cover all required split categories");

	std::map<std::uint64_t, NSSplit> assignments;
	for (const auto& [group, flags] : groupCoverage) {
		const auto ood = HasFlag(flags, NSCoverageFlags::OutOfDistribution);
		const auto stress = HasFlag(flags, NSCoverageFlags::StressCase);
	
		if (ood && stress)
			return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::SampleMismatch, "one leakage group cannot be both unseen-test and stress");
	
		assignments[group] = ood ? NSSplit::UnseenTest : stress ? NSSplit::Stress : (HashGroup(config.SplitSeed, group) % 10'000u < config.ValidationBasisPoints ? NSSplit::Validation : NSSplit::Train);
	}
	
	std::sort(entries.begin(), entries.end(), [](const auto& left, const auto& right) {
		return left.SampleId < right.SampleId;
	});
	
	std::set<std::uint64_t> groupsBySplit[4];
	
	for (auto& entry : entries) {
		const auto split = assignments.at(entry.LeakageGroupId);
		entry.Split = static_cast<std::uint32_t>(split);
		
		const auto splitIndex = entry.Split;
		++report.Header.SplitCounts[splitIndex];
		
		groupsBySplit[splitIndex].insert(entry.LeakageGroupId);
		
		for (std::uint32_t flag = 0u; flag < 6u; ++flag) {
			if ((entry.CoverageFlags & (1u << flag)) != 0u) ++report.CoverageSampleCounts[splitIndex][flag];
		}
		
		auto& stats = report.Visibility[splitIndex];
		AccumulateStats(stats, sampleVisibility.at(entry.SampleId));
	}
	
	for (std::uint32_t split = 0u; split < 4u; ++split) {
		if (report.Header.SplitCounts[split] == 0u)
			return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::Incomplete, "all four dataset splits must be nonempty");
		
		report.LeakageGroupCounts[split] = static_cast<std::uint32_t>(groupsBySplit[split].size());
		
		FinalizeWeights(report.Visibility[split]);
	}
	
	report.Header.DatasetId = config.DatasetId;
	report.Header.SplitSeed = config.SplitSeed;
	report.Header.EntryCount = static_cast<std::uint32_t>(entries.size());
	report.Header.ValidationBasisPoints = config.ValidationBasisPoints;
	report.Header.RequiredCoverageFlags = config.RequiredCoverageFlags;
	report.Header.LeakageGroupCount = static_cast<std::uint32_t>(groupCoverage.size());
	report.Header.DomainCount = static_cast<std::uint32_t>(domainOOD.size());
	return {};
}

NSSplitResult DatasetSplit::WriteManifest(const std::filesystem::path& path,const NSSplitReport& report,std::span<const NSSplitManifestEntry> entries) {
	
	if (entries.empty() || entries.size() != report.Header.EntryCount || entries.size() > SPLIT_MAX_ENTRY_COUNT)
		return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::InvalidArgument, "split manifest entries do not match report");
	
	auto header = report.Header;
	header.HeaderByteSize = sizeof(header);
	header.EntryRecordByteSize = sizeof(NSSplitManifestEntry);
	header.EntriesOffset = sizeof(header);
	header.FileByteSize = sizeof(header) + entries.size_bytes();
	header.CompletionMarker = SPLIT_COMPLETION_MARKER;
	header.ContentChecksum = NSNeuralShadowDataset::DatasetContainer::ComputeCRC32(std::as_bytes(entries));
	header.HeaderChecksum = HeaderChecksum(header);
	
	std::vector<std::byte> bytes(static_cast<std::size_t>(header.FileByteSize));
	
	std::memcpy(bytes.data(), &header, sizeof(header));
	std::memcpy(bytes.data() + header.EntriesOffset, entries.data(), entries.size_bytes());
	
	return AtomicWrite(path, bytes);
}

NSSplitResult DatasetSplit::LoadManifest(const std::filesystem::path& path,NSLoadedSplitManifest& manifest) {
	manifest = {};
	std::vector<std::byte> bytes;
	
	auto result = ReadFile(path, bytes);
	if (!result.Succeeded()) return result;
	
	NSSplitManifestHeader header{};
	std::memcpy(&header, bytes.data(), sizeof(header));
	
	if (header.Magic != SPLIT_MANIFEST_MAGIC || header.Version != SPLIT_MANIFEST_VERSION ||
		header.HeaderByteSize != sizeof(header) || header.EntryRecordByteSize != sizeof(NSSplitManifestEntry) ||
		header.FileByteSize != bytes.size() || header.DatasetId == 0u || header.SplitSeed == 0u ||
		header.EntryCount == 0u || header.EntryCount > SPLIT_MAX_ENTRY_COUNT ||
		header.EntriesOffset != sizeof(header) ||
		header.FileByteSize != sizeof(header) + static_cast<std::uint64_t>(header.EntryCount) * sizeof(NSSplitManifestEntry) ||
		header.PolicyVersion != SPLIT_POLICY_VERSION || header.ValidationBasisPoints == 0u ||
		header.ValidationBasisPoints >= 10'000u || header.CompletionMarker != SPLIT_COMPLETION_MARKER ||
		header.RequiredCoverageFlags != REQUIRED_COVERAGE_FLAGS ||
		header.HeaderChecksum != HeaderChecksum(header)) {
	
		return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::LayoutMismatch, "split manifest header is invalid");
	}
	
	for (const auto padding : header.Padding) {
		if (padding != 0u) {
			return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::LayoutMismatch, "split manifest padding is nonzero");
		}
	}
	
	const auto content = std::span<const std::byte>(bytes).subspan(header.EntriesOffset);
	if (header.ContentChecksum != NSNeuralShadowDataset::DatasetContainer::ComputeCRC32(content))
		return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::ChecksumMismatch, "split manifest content checksum mismatch");
	
	manifest.Header = header;
	manifest.Entries.resize(header.EntryCount);
	
	std::memcpy(manifest.Entries.data(), content.data(), content.size());
	
	std::uint64_t previous = 0u;
	std::uint32_t counts[4]{};
	
	std::map<std::uint64_t, std::uint32_t> groupSplits;
	std::map<std::uint64_t, std::uint64_t> sequenceGroups;
	
	std::set<std::uint64_t> domains;
	
	std::map<std::uint64_t, std::pair<bool, bool>> domainOOD;
	std::map<std::uint64_t, std::pair<bool, bool>> domainStress;
	
	std::uint32_t observedCoverage = 0u;
	
	for (const auto& entry : manifest.Entries) {
		if (entry.SampleId == 0u || entry.SampleId <= previous || entry.GenerationId == 0u ||
			entry.LeakageGroupId == 0u || entry.DomainId == 0u || entry.Split >= 4u ||
			(entry.CoverageFlags & ~KNOWN_COVERAGE_FLAGS) != 0u ||
			entry.Padding[0] != 0u || entry.Padding[1] != 0u || entry.Padding[2] != 0u) {
			return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::LayoutMismatch, "split manifest entry is invalid or unsorted");
		}
			
		previous = entry.SampleId;
		++counts[entry.Split];
		domains.insert(entry.DomainId);

		const auto [group, inserted] = groupSplits.emplace(entry.LeakageGroupId, entry.Split);
		
		if (!inserted && group->second != entry.Split) return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::SampleMismatch, "leakage group crosses split manifest partitions");
		
		if (entry.SequenceId != 0u) {
			const auto [sequence, insertedSequence] = sequenceGroups.emplace(entry.SequenceId, entry.LeakageGroupId);
			if (!insertedSequence && sequence->second != entry.LeakageGroupId) return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::SampleMismatch, "sequence crosses leakage groups");
		}

		const auto ood = HasFlag(entry.CoverageFlags, NSCoverageFlags::OutOfDistribution);
		const auto stress = HasFlag(entry.CoverageFlags, NSCoverageFlags::StressCase);
		
		if ((ood && entry.Split != static_cast<std::uint32_t>(NSSplit::UnseenTest)) ||
			(stress && entry.Split != static_cast<std::uint32_t>(NSSplit::Stress)) ||
			(!ood && entry.Split == static_cast<std::uint32_t>(NSSplit::UnseenTest)) ||
			(!stress && entry.Split == static_cast<std::uint32_t>(NSSplit::Stress))) {
			
			return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::SampleMismatch, "coverage flags do not match the assigned split");
		}
		
		auto& domainState = domainOOD[entry.DomainId];
		
		if (ood) domainState.first = true; else domainState.second = true;
		
		auto& domainStressState = domainStress[entry.DomainId];
		
		if (stress) domainStressState.first = true; else domainStressState.second = true;
		
		observedCoverage |= entry.CoverageFlags;
	}
	
	for (const auto& [domain, state] : domainOOD) {
		if (state.first && state.second) return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::SampleMismatch, "OOD domain is mixed in split manifest");
	}
	
	for (const auto& [domain, state] : domainStress) {
		if (state.first && state.second) return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::SampleMismatch, "stress domain is mixed in split manifest");
	}
	
	if (!std::equal(std::begin(counts), std::end(counts), std::begin(header.SplitCounts)) ||
		std::any_of(std::begin(counts), std::end(counts), [](std::uint32_t count) { return count == 0u; }) ||
		header.LeakageGroupCount != groupSplits.size() || header.DomainCount != domains.size() ||
		(observedCoverage & REQUIRED_COVERAGE_FLAGS) != REQUIRED_COVERAGE_FLAGS) {
		return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::LayoutMismatch, "split manifest counts do not match entries");
	}

	return {};
}

NSSplitResult DatasetSplit::WriteReportCSV(const std::filesystem::path& path,const NSSplitReport& report) {
	
	if (path.empty()) return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::InvalidArgument, "split report path is required");
	
	std::error_code error;
	if (!path.parent_path().empty()) std::filesystem::create_directories(path.parent_path(), error);
	
	if (error) return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::IoFailure, error.message());
	
	std::ostringstream text;
	
	text << "split_policy_version,dataset_id,split_seed,validation_basis_points,invalid_excluded,group_balanced,class_weight_policy,max_class_weight\n"
		<< SPLIT_POLICY_VERSION << ',' << report.Header.DatasetId << ',' << report.Header.SplitSeed << ','
		<< report.Header.ValidationBasisPoints << ",1,1,inverse_frequency_clamped," << SPLIT_MAX_VISIBILITY_CLASS_WEIGHT << "\n\n";
	
	text << "split,samples,leakage_groups,valid_values,fully_lit,fully_occluded,penumbra,invalid_values,"
		"fully_lit_weight,fully_occluded_weight,penumbra_weight,contact_samples,thin_samples,large_penumbra_samples,motion_samples,ood_samples,stress_samples\n";
	
	for (std::uint32_t split = 0u; split < 4u; ++split) {
		const auto& stats = report.Visibility[split];
		text << Name(static_cast<NSSplit>(split)) << ',' << report.Header.SplitCounts[split] << ','
			<< report.LeakageGroupCounts[split] << ',' << stats.ValidValueCount << ',' << stats.FullyLitValueCount << ','
			<< stats.FullyOccludedValueCount << ',' << stats.PenumbraValueCount << ',' << stats.InvalidValueCount << ','
			<< stats.FullyLitSamplingWeight << ',' << stats.FullyOccludedSamplingWeight << ','
			<< stats.PenumbraSamplingWeight;
		
		for (std::uint32_t flag = 0u; flag < 6u; ++flag) text << ',' << report.CoverageSampleCounts[split][flag];
		text << '\n';
	}
	
	const auto value = text.str();
	return AtomicWrite(path, std::as_bytes(std::span<const char>(value.data(), value.size())));
}

}
