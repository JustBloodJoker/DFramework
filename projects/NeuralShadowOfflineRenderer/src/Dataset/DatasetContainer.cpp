#include "pch.h"

#define _SILENCE_ALL_CXX20_DEPRECATION_WARNINGS
#include <Dataset/DatasetContainer.h>

namespace NSNeuralShadowDataset {

NSDatasetResult DatasetContainer::Failure(NSDatasetErrorCode code, std::string message) {
    return {code, std::move(message)};
}

bool DatasetContainer::CheckedAdd(std::uint64_t a, std::uint64_t b, std::uint64_t &result) {
    if (a > std::numeric_limits<std::uint64_t>::max() - b) return false;
    
	result = a + b;
    return true;
}

bool DatasetContainer::CheckedMultiply(std::uint64_t a, std::uint64_t b, std::uint64_t &result) {
    if (a != 0u && b > std::numeric_limits<std::uint64_t>::max() / a) return false;
    
	result = a * b;
    return true;
}

bool DatasetContainer::AlignUp16(std::uint64_t value, std::uint64_t &result) {
    std::uint64_t adjusted = 0u;
    if (!CheckedAdd(value, 15u, adjusted)) return false;
    
	result = adjusted & ~15ull;
    return true;
}

bool DatasetContainer::PathToUtf8(const std::filesystem::path &path, std::string &value) {
    const auto wide = path.generic_wstring();
    if (wide.empty()) {
        value.clear();
        return true;
    }

    const auto required = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide.data(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
    if (required <= 0) return false;

    value.resize(static_cast<std::size_t>(required));
    return WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide.data(), static_cast<int>(wide.size()), value.data(), required, nullptr, nullptr) == required;
}

bool DatasetContainer::Utf8ToPath(std::string_view value, std::filesystem::path &path) {
    if (value.empty()) {
        path.clear();
        return true;
    }

    const auto required = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), nullptr, 0);
    if (required <= 0) return false;

    std::wstring wide(static_cast<std::size_t>(required), L'\0');
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), wide.data(), required) !=
        required) {
        return false;
    }

    path = std::filesystem::path(std::move(wide));
    return true;
}

NSDatasetResult DatasetContainer::ReadFileLimited(const std::filesystem::path &path, std::uint64_t maximumBytes, std::vector<std::byte> &bytes) {
    std::error_code filesystemError;
	const auto size = std::filesystem::file_size(path, filesystemError);
    if (filesystemError) return Failure(NSDatasetErrorCode::IoFailure, filesystemError.message());
    
	if (size > maximumBytes || size > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        return Failure(NSDatasetErrorCode::FileTooLarge, "file exceeds the dataset allocation limit");
    }

    bytes.resize(static_cast<std::size_t>(size));
    std::ifstream stream(path, std::ios::binary);
    
	if (!stream) return Failure(NSDatasetErrorCode::IoFailure, "failed to open file");
    
	if (!bytes.empty()) stream.read(reinterpret_cast<char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));

    if (!stream || static_cast<std::size_t>(stream.gcount()) != bytes.size()) {
        return Failure(NSDatasetErrorCode::Truncated, "file ended before its declared filesystem size");
    }

    return {};
}

NSDatasetResult DatasetContainer::AtomicWriteFile(const std::filesystem::path &target, std::span<const std::byte> bytes) {
    
	std::error_code filesystemError;
    std::filesystem::create_directories(target.parent_path(), filesystemError);
    
	if (filesystemError) return Failure(NSDatasetErrorCode::IoFailure, filesystemError.message());

    static std::atomic<std::uint64_t> nextTemporaryId{1u};
    auto temporary = target;
    temporary += L".tmp." + std::to_wstring(GetCurrentProcessId()) + L"." +
                 std::to_wstring(nextTemporaryId.fetch_add(1u, std::memory_order_relaxed));
    
	{
        std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
        if (!stream) return Failure(NSDatasetErrorCode::IoFailure, "failed to create temporary file");
        
		if (!bytes.empty()) {
            stream.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        }

        stream.flush();
        if (!stream) {
            std::filesystem::remove(temporary, filesystemError);
            return Failure(NSDatasetErrorCode::IoFailure, "failed while writing temporary file");
        }
    }

    if (!MoveFileExW(temporary.c_str(), target.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        const auto win32Error = GetLastError();
        std::filesystem::remove(temporary, filesystemError);
        return Failure(NSDatasetErrorCode::IoFailure, "atomic replace failed with Win32 error " + std::to_string(win32Error));
    }

    return {};
}

template<class T>
std::uint32_t DatasetContainer::ComputeHeaderChecksum(T header) {
	header.HeaderChecksum = 0u;
	return ComputeCRC32(std::as_bytes(std::span<const T>(&header, 1u)));
}

NSDatasetResult DatasetContainer::ValidateStoredDatasetLayout(const FDW_SD::DatasetFormat::NSDatasetLayoutMetadata& layout) {
	const auto validation = FDW_SD::DatasetFormat::DatasetLayoutTools::ValidateDatasetMetadata(layout);
	if (validation.ErrorFlags == 0u) return {};
	
	return Failure(NSDatasetErrorCode::FormatMismatch, FDW_SD::DatasetFormat::DatasetLayoutTools::DescribeValidation(validation));
}

NSDatasetResult DatasetContainer::BuildSampleBytes(std::uint64_t sampleId,std::uint64_t generationId,std::span<const NSSampleBlockInput> blocks,std::vector<std::byte>& bytes) {
	
	if (sampleId == 0u || generationId == 0u || blocks.empty() ||
		blocks.size() > MAX_SAMPLE_BLOCK_COUNT) {
		return Failure(NSDatasetErrorCode::InvalidArgument, "sample id, generation, and bounded blocks are required");
	}

	std::set<std::uint32_t> kinds;
	for (const auto& block : blocks) {
		
		const auto kind = static_cast<std::uint32_t>(block.Kind);
		if (kind == 0u || block.Version == 0u) {
			return Failure(NSDatasetErrorCode::InvalidArgument, "block kind and version must be nonzero");
		}
		
		if (!kinds.insert(kind).second) {
			return Failure(NSDatasetErrorCode::DuplicateBlock, "sample block kinds must be unique");
		}
	}

	std::uint64_t tableBytes = 0u;
	std::uint64_t tableEnd = 0u;
	std::uint64_t payloadOffset = 0u;
	if (!CheckedMultiply(blocks.size(), sizeof(NSSampleBlockRecord), tableBytes) ||
		!CheckedAdd(sizeof(NSSampleFileHeader), tableBytes, tableEnd) ||
		!AlignUp16(tableEnd, payloadOffset)) {
		
		return Failure(NSDatasetErrorCode::BoundsViolation, "sample table size overflow");
	}

	std::vector<NSSampleBlockRecord> records(blocks.size());
	auto cursor = payloadOffset;
	for (std::size_t index = 0u; index < blocks.size(); ++index) {
		const auto& block = blocks[index];
		
		auto& record = records[index];
		record.Kind = static_cast<std::uint32_t>(block.Kind);
		record.Version = block.Version;
		record.Offset = cursor;
		record.ByteSize = block.Bytes.size();
		record.ElementCount = block.ElementCount;
		record.Checksum = ComputeCRC32(block.Bytes);
		
		std::uint64_t end = 0u;
		
		if (!CheckedAdd(cursor, record.ByteSize, end) || !AlignUp16(end, cursor)) {
			return Failure(NSDatasetErrorCode::BoundsViolation, "sample payload size overflow");
		}
	}
	
	if (cursor > MAX_SAMPLE_FILE_BYTES || cursor > std::numeric_limits<std::size_t>::max()) {
		return Failure(NSDatasetErrorCode::FileTooLarge, "sample exceeds the dataset file limit");
	}

	bytes.assign(static_cast<std::size_t>(cursor), std::byte{});
	
	NSSampleFileHeader header{};
	header.HeaderByteSize = sizeof(header);
	header.BlockRecordByteSize = sizeof(NSSampleBlockRecord);
	header.FileByteSize = cursor;
	header.SampleId = sampleId;
	header.GenerationId = generationId;
	header.BlockCount = static_cast<std::uint32_t>(blocks.size());
	header.LayoutOffset = offsetof(NSSampleFileHeader, Layout);
	header.BlockTableOffset = sizeof(header);
    header.PayloadOffset = payloadOffset;
    header.CompletionMarker = COMPLETION_MARKER;
    header.Layout = FDW_SD::DatasetFormat::DatasetLayoutTools::MakeDatasetMetadata();

    std::memcpy(bytes.data(), &header, sizeof(header));
    std::memcpy(bytes.data() + header.BlockTableOffset, records.data(), records.size() * sizeof(records.front()));
   
	for (std::size_t index = 0u; index < blocks.size(); ++index) {
        if (!blocks[index].Bytes.empty()) {
            std::memcpy(bytes.data() + records[index].Offset, blocks[index].Bytes.data(), blocks[index].Bytes.size());
        }
    }
    
	header.ContentChecksum = ComputeCRC32(std::span<const std::byte>(bytes.data() + header.BlockTableOffset, bytes.size() - static_cast<std::size_t>(header.BlockTableOffset)));
    header.HeaderChecksum = ComputeHeaderChecksum(header);
    
	std::memcpy(bytes.data(), &header, sizeof(header));
    return {};
}

NSDatasetResult DatasetContainer::BuildManifestBytes(std::uint64_t datasetId, std::span<const NSDatasetManifestEntry> inputEntries, std::vector<std::byte> &bytes) {
    if (datasetId == 0u || inputEntries.size() > MAX_MANIFEST_ENTRY_COUNT) {
        return Failure(NSDatasetErrorCode::InvalidArgument, "dataset id or entry count is invalid");
    }

    auto entries = std::vector<NSDatasetManifestEntry>(inputEntries.begin(), inputEntries.end());
    std::sort(entries.begin(), entries.end(), [](const auto &left, const auto &right) { return left.SampleId < right.SampleId; });

    std::set<std::string> paths;
    
	for (std::size_t index = 0u; index < entries.size(); ++index) {
        if (entries[index].SampleId == 0u || entries[index].GenerationId == 0u ||
            !IsSafeDatasetRelativePath(entries[index].RelativePath)) {
            
			return Failure(NSDatasetErrorCode::UnsafeRelativePath, "manifest contains an invalid sample identity or path");
        }
        
		if (index != 0u && entries[index - 1u].SampleId == entries[index].SampleId) {
            return Failure(NSDatasetErrorCode::ManifestConflict, "manifest sample ids must be unique");
        }
       
		std::string path;
        if (!PathToUtf8(entries[index].RelativePath, path) || path.empty() || path.size() > MAX_RELATIVE_PATH_BYTES ||
            !paths.insert(path).second) {
            
			return Failure(NSDatasetErrorCode::ManifestConflict, "manifest paths must be unique and bounded");
        }
    }

    std::uint64_t entryBytes = 0u;
    std::uint64_t pathsOffset = 0u;
    if (!CheckedMultiply(entries.size(), sizeof(NSDatasetManifestEntryRecord), entryBytes) ||
        !CheckedAdd(sizeof(NSDatasetManifestHeader), entryBytes, pathsOffset)) {
        
		return Failure(NSDatasetErrorCode::BoundsViolation, "manifest table size overflow");
    }
    
	auto cursor = pathsOffset;
    std::vector<std::string> encodedPaths;
    std::vector<NSDatasetManifestEntryRecord> records(entries.size());
	
	encodedPaths.reserve(entries.size());
	
	for (std::size_t index = 0u; index < entries.size(); ++index) {
		
		std::string encodedPath;
		if (!PathToUtf8(entries[index].RelativePath, encodedPath)) {
			return Failure(NSDatasetErrorCode::UnsafeRelativePath, "manifest path is not valid Unicode");
		}
		
		encodedPaths.push_back(std::move(encodedPath));
		
		auto& record = records[index];
		record.SampleId = entries[index].SampleId;
		record.GenerationId = entries[index].GenerationId;
		record.SampleByteSize = entries[index].SampleByteSize;
		record.PathOffset = cursor;
		record.PathByteSize = static_cast<std::uint32_t>(encodedPaths.back().size());
		record.SampleChecksum = entries[index].SampleChecksum;
		
		if (!CheckedAdd(cursor, record.PathByteSize, cursor)) {
			return Failure(NSDatasetErrorCode::BoundsViolation, "manifest path size overflow");
		}
	}
	
	if (cursor > MAX_MANIFEST_FILE_BYTES || cursor > std::numeric_limits<std::size_t>::max()) {
		return Failure(NSDatasetErrorCode::FileTooLarge, "manifest exceeds the dataset file limit");
	}

	bytes.assign(static_cast<std::size_t>(cursor), std::byte{});
	
	NSDatasetManifestHeader header{};
	header.HeaderByteSize = sizeof(header);
	header.EntryRecordByteSize = sizeof(NSDatasetManifestEntryRecord);
	header.FileByteSize = cursor;
	header.DatasetId = datasetId;
	header.EntryCount = static_cast<std::uint32_t>(entries.size());
	header.EntriesOffset = sizeof(header);
	header.PathsOffset = pathsOffset;
	header.CompletionMarker = COMPLETION_MARKER;
	header.Layout = FDW_SD::DatasetFormat::DatasetLayoutTools::MakeDatasetMetadata();
	if (!records.empty()) {
		std::memcpy(bytes.data() + header.EntriesOffset, records.data(), records.size() * sizeof(records.front()));
	}
	
	for (std::size_t index = 0u; index < records.size(); ++index) {
		std::memcpy(bytes.data() + records[index].PathOffset, encodedPaths[index].data(), encodedPaths[index].size());
    }
    
	header.ContentChecksum = ComputeCRC32(std::span<const std::byte>(bytes.data() + header.EntriesOffset, bytes.size() - static_cast<std::size_t>(header.EntriesOffset)));
    header.HeaderChecksum = ComputeHeaderChecksum(header);
    
	std::memcpy(bytes.data(), &header, sizeof(header));
    return {};
}

std::uint32_t DatasetContainer::ComputeCRC32(std::span<const std::byte> bytes) {
    std::uint32_t crc = 0xffff'ffffu;
    for (const auto byte : bytes) {
        crc ^= static_cast<std::uint8_t>(byte);
        for (std::uint32_t bit = 0u; bit < 8u; ++bit) {
            crc = (crc >> 1u) ^ (0xedb8'8320u & (0u - (crc & 1u)));
        }
    }
    return ~crc;
}

bool DatasetContainer::IsSafeDatasetRelativePath(const std::filesystem::path &relativePath) {
    if (relativePath.empty() || relativePath.is_absolute() || relativePath.has_root_name() || relativePath.has_root_directory()) return false;
    
	for (const auto &component : relativePath) {
        if (component.empty() || component == "." || component == "..") return false;
    }
    std::string encoded;
    
	if (!PathToUtf8(relativePath, encoded)) return false;
    
	if (encoded.empty() || encoded.size() > MAX_RELATIVE_PATH_BYTES) return false;
    
	for (const auto character : encoded) {
        const auto value = static_cast<unsigned char>(character);
        if (value < 0x20u || character == '<' || character == '>' || character == ':' || character == '"' || character == '|' ||
            character == '?' || character == '*')
            
			return false;
    }
    return true;
}

NSDatasetResult DatasetContainer::LoadSampleFile(const std::filesystem::path& datasetRoot,const std::filesystem::path& relativePath,NSLoadedSample& sample) {
	sample = {};
	if (!IsSafeDatasetRelativePath(relativePath)) {
		return Failure(NSDatasetErrorCode::UnsafeRelativePath, "sample path must be relative to the dataset root");
	}

	std::vector<std::byte> bytes;
	auto result = ReadFileLimited(datasetRoot / relativePath, MAX_SAMPLE_FILE_BYTES, bytes);
	
	if (!result.Succeeded()) return result;
	
	if (bytes.size() < sizeof(NSSampleFileHeader)) {
		return Failure(NSDatasetErrorCode::Truncated, "sample header is truncated");
	}

	NSSampleFileHeader header{};
	std::memcpy(&header, bytes.data(), sizeof(header));
	
	if (header.Magic != SAMPLE_FILE_MAGIC) return Failure(NSDatasetErrorCode::IdentityMismatch, "sample magic mismatch");
	
	if (header.Version != SAMPLE_FILE_VERSION) return Failure(NSDatasetErrorCode::VersionMismatch, "sample version mismatch");
	
	if (header.HeaderByteSize != sizeof(header) ||
		header.BlockRecordByteSize != sizeof(NSSampleBlockRecord) ||
		header.LayoutOffset != offsetof(NSSampleFileHeader, Layout) ||
		header.BlockTableOffset != sizeof(header) || header.Padding0 != 0u || header.Flags != 0u) {
		
		return Failure(NSDatasetErrorCode::LayoutMismatch, "sample fixed layout mismatch");
	}
	
	if (header.FileByteSize != bytes.size()) return Failure(NSDatasetErrorCode::Truncated, "sample declared size mismatch");
	
	if (header.SampleId == 0u || header.GenerationId == 0u) return Failure(NSDatasetErrorCode::SampleMismatch, "sample identity is zero");
	
	if (header.CompletionMarker != COMPLETION_MARKER) return Failure(NSDatasetErrorCode::Incomplete, "sample completion marker is absent");
	
	if (header.HeaderChecksum != ComputeHeaderChecksum(header)) return Failure(NSDatasetErrorCode::ChecksumMismatch, "sample header checksum mismatch");
	
	if (header.ContentChecksum != ComputeCRC32(std::span<const std::byte>(
		bytes.data() + header.BlockTableOffset,
		bytes.size() - static_cast<std::size_t>(header.BlockTableOffset)))) {
	
		return Failure(NSDatasetErrorCode::ChecksumMismatch, "sample content checksum mismatch");
	}

	result = ValidateStoredDatasetLayout(header.Layout);
	if (!result.Succeeded()) return result;
	
	if (header.BlockCount == 0u || header.BlockCount > MAX_SAMPLE_BLOCK_COUNT) {
		return Failure(NSDatasetErrorCode::LayoutMismatch, "sample block count is invalid");
	}

	std::uint64_t tableBytes = 0u;
	std::uint64_t tableEnd = 0u;
	if (!CheckedMultiply(header.BlockCount, sizeof(NSSampleBlockRecord), tableBytes) ||
		!CheckedAdd(header.BlockTableOffset, tableBytes, tableEnd) ||
		tableEnd > bytes.size() || header.PayloadOffset < tableEnd || header.PayloadOffset > bytes.size()) {
		
		return Failure(NSDatasetErrorCode::BoundsViolation, "sample block table is outside the file");
	}

	std::set<std::uint32_t> kinds;
	std::uint64_t previousEnd = header.PayloadOffset;
	
	sample.Header = header;
	sample.Blocks.reserve(header.BlockCount);
	
	for (std::uint32_t index = 0u; index < header.BlockCount; ++index) {
		NSSampleBlockRecord record{};
		
		std::memcpy(&record, bytes.data() + header.BlockTableOffset + index * sizeof(record), sizeof(record));
		std::uint64_t end = 0u;
		
		if (record.Kind == 0u || record.Version == 0u || record.Flags != 0u ||
			!CheckedAdd(record.Offset, record.ByteSize, end) || record.Offset < header.PayloadOffset ||
			end > bytes.size()) {
			
			return Failure(NSDatasetErrorCode::BoundsViolation, "sample block is outside the payload");
		}

		if (record.Offset < previousEnd) return Failure(NSDatasetErrorCode::OverlappingBlocks, "sample blocks overlap or are unsorted");
		
		if (!kinds.insert(record.Kind).second) return Failure(NSDatasetErrorCode::DuplicateBlock, "sample block kind is duplicated");
		
		const auto blockBytes = std::span<const std::byte>(bytes.data() + record.Offset, static_cast<std::size_t>(record.ByteSize));
		
		if (ComputeCRC32(blockBytes) != record.Checksum) {
			return Failure(NSDatasetErrorCode::ChecksumMismatch, "sample block checksum mismatch");
		}

		NSLoadedSampleBlock block{};
		block.Record = record;
		block.Bytes.assign(blockBytes.begin(), blockBytes.end());
		
		sample.Blocks.push_back(std::move(block));
		
		previousEnd = end;
	}
	return {};
}

NSDatasetResult DatasetContainer::LoadDatasetManifest(const std::filesystem::path& datasetRoot,NSLoadedDatasetManifest& manifest) {
	
	manifest = {};
	std::vector<std::byte> bytes;
	
	auto result = ReadFileLimited(datasetRoot / MANIFEST_FILENAME, MAX_MANIFEST_FILE_BYTES, bytes);
	if (!result.Succeeded()) return result;
	
	if (bytes.size() < sizeof(NSDatasetManifestHeader)) {
		return Failure(NSDatasetErrorCode::Truncated, "manifest header is truncated");
	}
	
	NSDatasetManifestHeader header{};
	std::memcpy(&header, bytes.data(), sizeof(header));
	
	if (header.Magic != MANIFEST_FILE_MAGIC) return Failure(NSDatasetErrorCode::IdentityMismatch, "manifest magic mismatch");
	
	if (header.Version != MANIFEST_FILE_VERSION) return Failure(NSDatasetErrorCode::VersionMismatch, "manifest version mismatch");
	
	if (header.HeaderByteSize != sizeof(header) ||
		header.EntryRecordByteSize != sizeof(NSDatasetManifestEntryRecord) ||
		header.EntriesOffset != sizeof(header) || header.Flags != 0u || header.Padding0 != 0u) {
		
		return Failure(NSDatasetErrorCode::LayoutMismatch, "manifest fixed layout mismatch");
	}
	
	if (header.FileByteSize != bytes.size()) return Failure(NSDatasetErrorCode::Truncated, "manifest declared size mismatch");
	
	if (header.DatasetId == 0u) return Failure(NSDatasetErrorCode::IdentityMismatch, "manifest dataset id is zero");
	
	if (header.CompletionMarker != COMPLETION_MARKER) return Failure(NSDatasetErrorCode::Incomplete, "manifest completion marker is absent");
	
	if (header.HeaderChecksum != ComputeHeaderChecksum(header)) return Failure(NSDatasetErrorCode::ChecksumMismatch, "manifest header checksum mismatch");
	
	result = ValidateStoredDatasetLayout(header.Layout);
	
	if (!result.Succeeded()) return result;
	
	if (header.EntryCount > MAX_MANIFEST_ENTRY_COUNT) return Failure(NSDatasetErrorCode::LayoutMismatch, "manifest entry count exceeds the supported limit");

	std::uint64_t entryBytes = 0u;
	std::uint64_t entriesEnd = 0u;
	if (!CheckedMultiply(header.EntryCount, sizeof(NSDatasetManifestEntryRecord), entryBytes) ||
		!CheckedAdd(header.EntriesOffset, entryBytes, entriesEnd) || entriesEnd != header.PathsOffset ||
		header.PathsOffset > bytes.size()) {
		
		return Failure(NSDatasetErrorCode::BoundsViolation, "manifest entry table is outside the file");
	}
	const auto content = std::span<const std::byte>(bytes.data() + header.EntriesOffset, bytes.size() - static_cast<std::size_t>(header.EntriesOffset));
	if (ComputeCRC32(content) != header.ContentChecksum) {
		return Failure(NSDatasetErrorCode::ChecksumMismatch, "manifest content checksum mismatch");
	}

	std::set<std::uint64_t> sampleIds;
	std::set<std::string> paths;
	std::uint64_t previousPathEnd = header.PathsOffset;
	std::uint64_t previousSampleId = 0u;
	
	manifest.Header = header;
	manifest.Entries.reserve(header.EntryCount);
	
	for (std::uint32_t index = 0u; index < header.EntryCount; ++index) {
		
		NSDatasetManifestEntryRecord record{};
        std::memcpy(&record, bytes.data() + header.EntriesOffset + index * sizeof(record), sizeof(record));
        std::uint64_t pathEnd = 0u;
       
		if (record.SampleId == 0u || record.GenerationId == 0u || record.Flags != 0u || record.Padding0 != 0u ||
            record.PathByteSize == 0u || record.PathByteSize > MAX_RELATIVE_PATH_BYTES ||
            !CheckedAdd(record.PathOffset, record.PathByteSize, pathEnd) || record.PathOffset < header.PathsOffset ||
            pathEnd > bytes.size() || record.PathOffset < previousPathEnd) {
           
			return Failure(NSDatasetErrorCode::BoundsViolation, "manifest path record is invalid");
        }
        
		if (index != 0u && record.SampleId <= previousSampleId) {
            return Failure(NSDatasetErrorCode::LayoutMismatch, "manifest entries are not strictly sorted by sample id");
        }
        
		const std::string encodedPath(reinterpret_cast<const char *>(bytes.data() + record.PathOffset), record.PathByteSize);
        
		std::filesystem::path relativePath;
        if (!Utf8ToPath(encodedPath, relativePath)) {
            return Failure(NSDatasetErrorCode::UnsafeRelativePath, "manifest path is not valid UTF-8");
        }
        
		if (!IsSafeDatasetRelativePath(relativePath)) {
            return Failure(NSDatasetErrorCode::UnsafeRelativePath, "manifest path escapes the dataset root");
        }
       
		if (!sampleIds.insert(record.SampleId).second || !paths.insert(encodedPath).second) {
            return Failure(NSDatasetErrorCode::ManifestConflict, "manifest contains duplicate identity or path");
        }
        
		manifest.Entries.push_back({
			record.SampleId,
			record.GenerationId,
			record.SampleByteSize,
			record.SampleChecksum,
			relativePath
		});
		
		previousSampleId = record.SampleId;
		previousPathEnd = pathEnd;
	}
	return {};
}

NSDatasetResult DatasetContainer::PublishDatasetSample(const std::filesystem::path& datasetRoot,std::uint64_t datasetId,const std::filesystem::path& relativePath,
													   std::uint64_t sampleId,std::uint64_t generationId,std::span<const NSSampleBlockInput> blocks) {

	if (datasetRoot.empty() || datasetId == 0u || !IsSafeDatasetRelativePath(relativePath)) {
		return Failure(NSDatasetErrorCode::UnsafeRelativePath, "dataset root/id and safe relative sample path are required");
	}
	
	NSLoadedDatasetManifest manifest{};
	std::vector<NSDatasetManifestEntry> entries;
	std::error_code filesystemError;
	
	const auto manifestPath = datasetRoot / MANIFEST_FILENAME;
	if (std::filesystem::exists(manifestPath, filesystemError)) {
		auto result = LoadDatasetManifest(datasetRoot, manifest);
		if (!result.Succeeded()) return result;
		
		if (manifest.Header.DatasetId != datasetId) {
			return Failure(NSDatasetErrorCode::ManifestConflict, "dataset id differs from the existing manifest");
		}
		
		entries = manifest.Entries;
	}
	else if (filesystemError) {
		return Failure(NSDatasetErrorCode::IoFailure, filesystemError.message());
	}

	for (const auto& entry : entries) {
		if (entry.SampleId == sampleId && entry.RelativePath != relativePath) {
			return Failure(NSDatasetErrorCode::ManifestConflict, "sample id is already bound to another path");
		}
		
		if (entry.SampleId != sampleId && entry.RelativePath == relativePath) {
			return Failure(NSDatasetErrorCode::ManifestConflict, "sample path is already bound to another id");
		}
	}

	std::vector<std::byte> sampleBytes;
	auto result = BuildSampleBytes(sampleId, generationId, blocks, sampleBytes);
	if (!result.Succeeded()) return result;
	
	result = AtomicWriteFile(datasetRoot / relativePath, sampleBytes);
	
	if (!result.Succeeded()) return result;

	NSDatasetManifestEntry replacement{
		sampleId,
		generationId,
		sampleBytes.size(),
		ComputeCRC32(sampleBytes),
		relativePath
	};
	
	auto existing = std::find_if(entries.begin(), entries.end(), [sampleId](const auto& entry) {
		return entry.SampleId == sampleId;
	});
	
	if (existing == entries.end()) entries.push_back(replacement);
	else *existing = replacement;

	std::vector<std::byte> manifestBytes;
	result = BuildManifestBytes(datasetId, entries, manifestBytes);
	
	if (!result.Succeeded()) return result;
	
	return AtomicWriteFile(manifestPath, manifestBytes);
}

NSRegenerationStatus DatasetContainer::GetSampleRegenerationStatus(const std::filesystem::path& datasetRoot,std::uint64_t datasetId,const std::filesystem::path& relativePath,
																   std::uint64_t sampleId,std::uint64_t generationId,NSDatasetResult& detail) {
	
	detail = {};
	
	if (datasetId == 0u || sampleId == 0u || generationId == 0u ||
		!IsSafeDatasetRelativePath(relativePath)) {
		
		detail = Failure(NSDatasetErrorCode::UnsafeRelativePath, "regeneration identity/path is invalid");
		return NSRegenerationStatus::Conflict;
	}
	
	std::error_code filesystemError;
	if (!std::filesystem::exists(datasetRoot / MANIFEST_FILENAME, filesystemError)) {
		
		if (filesystemError) detail = Failure(NSDatasetErrorCode::IoFailure, filesystemError.message());
		else detail.Message = "manifest is missing";
		
		return filesystemError ? NSRegenerationStatus::Conflict : NSRegenerationStatus::GenerateMissing;
	}
	
	NSLoadedDatasetManifest manifest{};
	detail = LoadDatasetManifest(datasetRoot, manifest);
	if (!detail.Succeeded() || manifest.Header.DatasetId != datasetId) {
		
		if (detail.Succeeded()) detail = Failure(NSDatasetErrorCode::ManifestConflict, "dataset id mismatch");
		
		return NSRegenerationStatus::Conflict;
	}

	const NSDatasetManifestEntry* byId = nullptr;
	const NSDatasetManifestEntry* byPath = nullptr;
	for (const auto& entry : manifest.Entries) {
		
		if (entry.SampleId == sampleId) byId = &entry;
		
		if (entry.RelativePath == relativePath) byPath = &entry;
	}
	
	if (!byId && !byPath) {
		detail = {};
		detail.Message = "sample is absent";
		return NSRegenerationStatus::GenerateMissing;
	}
	
	if (!byId || !byPath || byId != byPath) {
		detail = Failure(NSDatasetErrorCode::ManifestConflict, "sample id/path identity conflicts with the manifest");
		return NSRegenerationStatus::Conflict;
	}
	
	if (byId->GenerationId != generationId) {
		detail = Failure(NSDatasetErrorCode::SampleMismatch, "sample generation changed");
		return NSRegenerationStatus::RegenerateInvalid;
	}
	
	NSLoadedSample sample{};
	detail = LoadSampleFile(datasetRoot, relativePath, sample);
	if (!detail.Succeeded() || sample.Header.SampleId != sampleId || sample.Header.GenerationId != generationId) {
		
		if (detail.Succeeded()) detail = Failure(NSDatasetErrorCode::SampleMismatch, "sample header identity mismatch");
		
		return NSRegenerationStatus::RegenerateInvalid;
	}
	
	std::vector<std::byte> bytes;
	detail = ReadFileLimited(datasetRoot / relativePath, MAX_SAMPLE_FILE_BYTES, bytes);
	if (!detail.Succeeded() || byId->SampleByteSize != bytes.size() ||
		byId->SampleChecksum != ComputeCRC32(bytes)) {
		
		if (detail.Succeeded()) detail = Failure(NSDatasetErrorCode::ChecksumMismatch, "sample differs from its manifest entry");
		
		return NSRegenerationStatus::RegenerateInvalid;
	}
	
	detail = {};
	detail.Message = "sample is complete and valid";
	return NSRegenerationStatus::SkipValid;
}

} // namespace NSNeuralShadowDataset
