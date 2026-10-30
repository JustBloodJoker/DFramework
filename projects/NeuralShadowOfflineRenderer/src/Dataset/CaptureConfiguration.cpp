#include "pch.h"


#define _SILENCE_ALL_CXX20_DEPRECATION_WARNINGS
#include <ShadowData/CaptureConfiguration.h>

#include <ShadowData/NeuralShadowData.h>

#include <cerrno>
#include <cstdlib>


namespace FDW_SD::Capture {

	bool NSCaptureOwnedBlock::HasLayout(std::uint32_t version, std::uint64_t elementCount, std::uint64_t byteCount) const {
		return Version == version && ElementCount == elementCount && Bytes.size() == byteCount;
	}

	NSCaptureResult CaptureConfiguration::Failure(NSNeuralShadowDataset::NSDatasetErrorCode code, std::string message) {
		return { code, std::move(message) };
	}

	bool CaptureConfiguration::IsFinite(float value) {
		return std::isfinite(value);
	}

	bool CaptureConfiguration::IsFinite(NSCaptureFloat3 value) {
		return IsFinite(value.X) && IsFinite(value.Y) && IsFinite(value.Z);
	}

	float CaptureConfiguration::DecodeFloat16(std::uint16_t bits) {
		const auto sign = (bits & 0x8000u) != 0u ? -1.0f : 1.0f;
		const auto exponent = static_cast<std::uint32_t>((bits >> 10u) & 0x1fu);
		const auto mantissa = static_cast<std::uint32_t>(bits & 0x03ffu);
		
		if (exponent == 0u) return sign * std::ldexp(static_cast<float>(mantissa), -24);
		
		if (exponent == 31u) return mantissa == 0u ? sign * std::numeric_limits<float>::infinity() : std::numeric_limits<float>::quiet_NaN();

		return sign * std::ldexp(1.0f + static_cast<float>(mantissa) / 1024.0f, static_cast<int>(exponent) - 15);
	}

	bool CaptureConfiguration::IsValidRange(NSCaptureUIntRange range, std::uint32_t maximum) {
		return range.Minimum <= range.Maximum && range.Maximum <= maximum;
	}

	bool CaptureConfiguration::IsValidRange(NSCaptureFloatRange range, float minimum, float maximum) {
		return IsFinite(range.Minimum) && IsFinite(range.Maximum) && range.Minimum <= range.Maximum && range.Minimum >= minimum && range.Maximum <= maximum;
	}

	bool CaptureConfiguration::IsValidRange(NSCaptureFloat3Range range) {
		return IsFinite(range.Minimum) && IsFinite(range.Maximum) && range.Minimum.X <= range.Maximum.X && range.Minimum.Y <= range.Maximum.Y && range.Minimum.Z <= range.Maximum.Z;
	}

	std::string CaptureConfiguration::Trim(std::string value) {
		const auto first = value.find_first_not_of(" \t\r\n");
		if (first == std::string::npos) return {};
		
		const auto last = value.find_last_not_of(" \t\r\n");
		return value.substr(first, last - first + 1u);
	}

	template<class T>
	bool CaptureConfiguration::ParseUnsigned(std::string_view value, T& result) {
		if (value.empty()) return false;
		
		T parsed{};
		
		const auto conversion = std::from_chars(value.data(), value.data() + value.size(), parsed);
		if (conversion.ec != std::errc{} || conversion.ptr != value.data() + value.size()) return false;
		
		result = parsed;
		return true;
	}

	bool CaptureConfiguration::ParseFloat(std::string_view value, float& result) {
		std::string owned(value);
		
		char* end = nullptr;
		errno = 0;
		const auto parsed = std::strtof(owned.c_str(), &end);
		
		if (errno == ERANGE || end != owned.c_str() + owned.size() || !IsFinite(parsed)) return false;
		
		result = parsed;
		return true;
	}

	bool CaptureConfiguration::ParseFloat3(std::string_view value, NSCaptureFloat3& result) {
		const auto first = value.find(',');
		if (first == std::string_view::npos) return false;
		
		const auto second = value.find(',', first + 1u);
		if (second == std::string_view::npos || value.find(',', second + 1u) != std::string_view::npos) return false;
		
		return ParseFloat(value.substr(0u, first), result.X) && ParseFloat(value.substr(first + 1u, second - first - 1u), result.Y) && ParseFloat(value.substr(second + 1u), result.Z);
	}

	std::uint32_t CaptureConfiguration::PcgHash(std::uint32_t input) {
		const std::uint32_t state = input * 747796405u + 2891336453u;
		const std::uint32_t word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
		return (word >> 22u) ^ word;
	}

	std::uint32_t CaptureConfiguration::RandomBits(std::uint32_t sampleSeed, std::uint32_t domain, std::uint32_t object, std::uint32_t dimension) {
		auto value = PcgHash(sampleSeed ^ (0x9e37'79b9u * (domain + 1u)));
		value = PcgHash(value ^ object);
		return PcgHash(value ^ (0x9e37'79b9u * (dimension + 1u)));
	}

	float CaptureConfiguration::RandomUnit(std::uint32_t sampleSeed, std::uint32_t domain, std::uint32_t object, std::uint32_t dimension) {
		return static_cast<float>((static_cast<double>(RandomBits(sampleSeed, domain, object, dimension)) + 0.5) / 4294967296.0);
	}

	float CaptureConfiguration::RandomRange(std::uint32_t seed, std::uint32_t domain, std::uint32_t object, std::uint32_t dimension, NSCaptureFloatRange range) {
		return range.Minimum + (range.Maximum - range.Minimum) * RandomUnit(seed, domain, object, dimension);
	}

	NSCaptureFloat3 CaptureConfiguration::RandomRange( std::uint32_t seed, std::uint32_t domain,std::uint32_t object, std::uint32_t firstDimension, NSCaptureFloat3Range range) {
		
		return {
			range.Minimum.X + (range.Maximum.X - range.Minimum.X) * RandomUnit(seed, domain, object, firstDimension),
			range.Minimum.Y + (range.Maximum.Y - range.Minimum.Y) * RandomUnit(seed, domain, object, firstDimension + 1u),
			range.Minimum.Z + (range.Maximum.Z - range.Minimum.Z) * RandomUnit(seed, domain, object, firstDimension + 2u)
		};
	}

	std::uint32_t CaptureConfiguration::RandomCount(std::uint32_t seed,std::uint32_t domain,NSCaptureUIntRange range) {
		
		const auto span = static_cast<std::uint64_t>(range.Maximum) - range.Minimum + 1u;
		return range.Minimum + static_cast<std::uint32_t>(RandomBits(seed, domain, 0u, 0u) % span);
	}

	std::uint32_t CaptureConfiguration::SelectMaskedType(std::uint32_t bits, std::uint32_t mask, std::uint32_t typeCount) {
		std::uint32_t enabled[9]{};
		std::uint32_t count = 0u;
		
		for (std::uint32_t index = 0u; index < typeCount; ++index) {
			if ((mask & (1u << index)) != 0u) enabled[count++] = index;
		}
		
		return count == 0u ? 0u : enabled[bits % count];
	}

	std::vector<std::string> CaptureConfiguration::SplitSceneAssets(std::string_view value) {
		std::vector<std::string> assets;
		std::size_t begin = 0u;
		while (begin <= value.size()) {
			const auto end = value.find(';', begin);
			auto asset = Trim(std::string(value.substr(begin, end == std::string_view::npos ? value.size() - begin : end - begin)));
			
			if (asset.empty()) return {};
			
			assets.push_back(std::move(asset));
		
			if (end == std::string_view::npos) break;
			
			begin = end + 1u;
		}

		return assets;
	}

	NSCaptureFloat3 CaptureConfiguration::RandomUnitDirection(std::uint32_t seed, std::uint32_t object) {
		const auto z = RandomUnit(seed, 4u, object, 0u) * 2.0f - 1.0f;
		const auto angle = RandomUnit(seed, 4u, object, 1u) * 2.0f * PI;
		const auto radius = std::sqrt(std::max(0.0f, 1.0f - z * z));
		return { radius * std::cos(angle), z, radius * std::sin(angle) };
	}

	NSCaptureFloat3 CaptureConfiguration::DirectionFromTo(NSCaptureFloat3 from, NSCaptureFloat3 to) {
		const auto x = to.X - from.X;
		const auto y = to.Y - from.Y;
		const auto z = to.Z - from.Z;
		const auto length = std::sqrt(x * x + y * y + z * z);
		return length > 1.0e-6f ? NSCaptureFloat3{ x / length, y / length, z / length } : NSCaptureFloat3{ 0.0f, -1.0f, 0.0f };
	}

	std::uint64_t CaptureConfiguration::HashBytes(std::uint64_t hash, const void* data, std::size_t size) {
		const auto* bytes = static_cast<const unsigned char*>(data);
		
		for (std::size_t index = 0u; index < size; ++index) {
			hash = (hash ^ bytes[index]) * FNV_PRIME;
		}

		return hash;
	}

	template<class T>
	std::uint64_t CaptureConfiguration::HashValue(std::uint64_t hash, const T& value) {
		return HashBytes(hash, &value, sizeof(value));
	}

	std::uint64_t CaptureConfiguration::MakeSampleId(std::uint64_t datasetId, std::uint64_t configHash, std::uint32_t sampleIndex) {
		auto hash = FNV_OFFSET;
		hash = HashValue(hash, datasetId);
		hash = HashValue(hash, configHash);
		hash = HashValue(hash, sampleIndex);
		return hash == 0u ? 1u : hash;
	}

	std::filesystem::path CaptureConfiguration::MakeSamplePath(std::uint32_t sampleIndex) {
		std::ostringstream name;
		name << "samples/" << std::setfill('0') << std::setw(8) << sampleIndex << ".nssample";
		return std::filesystem::u8path( name.str() );
	}

	std::uint32_t CaptureConfiguration::Align16(std::uint32_t value) {
		return (value + 15u) & ~15u;
	}

	std::vector<std::byte> CaptureConfiguration::SerializeMetadata(NSCaptureMetadataHeader& header, const std::string& scene,std::span<const NSCaptureGeometryMetadata> geometry, std::span<const NSCaptureLightMetadata> lights) {
		
		header.HeaderByteSize = sizeof(header);
		header.ScenePathOffset = sizeof(header);
		header.ScenePathByteSize = static_cast<std::uint32_t>(scene.size());
		header.GeometryOffset = Align16(header.ScenePathOffset + header.ScenePathByteSize);
		header.GeometryCount = static_cast<std::uint32_t>(geometry.size());
		header.GeometryStride = sizeof(NSCaptureGeometryMetadata);
		header.LightOffset = Align16(header.GeometryOffset + header.GeometryCount * header.GeometryStride);
		header.LightCount = static_cast<std::uint32_t>(lights.size());
		header.LightStride = sizeof(NSCaptureLightMetadata);
		header.FileByteSize = header.LightOffset + header.LightCount * header.LightStride;
		
		std::vector<std::byte> bytes(header.FileByteSize);
		
		std::memcpy(bytes.data(), &header, sizeof(header));
		std::memcpy(bytes.data() + header.ScenePathOffset, scene.data(), scene.size());
		
		if (!geometry.empty()) {
			std::memcpy(bytes.data() + header.GeometryOffset, geometry.data(), geometry.size_bytes());
		}
		
		if (!lights.empty()) {
			std::memcpy(bytes.data() + header.LightOffset, lights.data(), lights.size_bytes());
		}

		return bytes;
	}

	template<class T>
	bool CaptureConfiguration::ReadStruct(std::span<const std::byte> bytes, T& value) {
		if (bytes.size() != sizeof(T)) return false;
		
		std::memcpy(&value, bytes.data(), sizeof(T));
		return true;
	}

	const NSCaptureOwnedBlock* CaptureConfiguration::FindBlock(std::span<const NSCaptureOwnedBlock> blocks,NSNeuralShadowDataset::NSSampleBlockKind kind) {
		const NSCaptureOwnedBlock* found = nullptr;
		
		for (const auto& block : blocks) {
		
			if (block.Kind != kind) continue;
			
			if (found) return nullptr;
			
			found = &block;
		}
		return found;
	}

	bool NSCaptureResult::Succeeded() const {
		return Code == NSNeuralShadowDataset::NSDatasetErrorCode::None;
	}

	std::uint32_t CaptureConfiguration::RequiredValidTargetCount(const NSCaptureSamplePlan&) {
		return CAPTURE_MIN_VALID_TARGETS;
	}

	NSCaptureResult CaptureConfiguration::Validate(const NSCaptureConfig& config) {
		if (config.Version != CAPTURE_CONFIG_FORMAT_VERSION) {
			return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::VersionMismatch, "capture config uses an unsupported format");
		}
	
		if (config.DatasetId == 0u || config.SampleCount == 0u ||
			config.SampleCount > CAPTURE_MAX_SAMPLE_COUNT || config.GenerationBase == 0u ||
			config.GenerationBase > std::numeric_limits<std::uint64_t>::max() - (config.SampleCount - 1u)) {
	
			return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::InvalidArgument, "dataset/sample/generation identity is invalid or unbounded");
		}
	
		if (config.SceneAsset.empty() || config.SceneAsset.size() > 4096u || SplitSceneAssets(config.SceneAsset).empty() ||
			config.OutputWidth == 0u || config.OutputHeight == 0u ||
			config.TargetSamplesPerPixel < NS_RT_OFFLINE_FINITE_SAMPLE_COUNT ||
			config.TargetSamplesPerPixel > NS_RT_MAX_FINITE_SAMPLE_COUNT) {
	
			return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::InvalidArgument, "scene/output identity or target_spp [256,4096] is invalid");
		}

		if (!IsValidRange(config.GeometryCount, CAPTURE_MAX_GEOMETRY_COUNT) ||
			!IsValidRange(config.LightCount, CAPTURE_MAX_LIGHT_COUNT) || config.LightCount.Minimum == 0u ||
			(config.GeometryTypeMask & 0x01ffu) == 0u || (config.GeometryTypeMask & ~0x01ffu) != 0u ||
			(config.LightTypeMask & 0x0fu) == 0u || (config.LightTypeMask & ~0x0fu) != 0u) {
	
			return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::BoundsViolation, "geometry/light count range is invalid");
		}

		if (!IsValidRange(config.GeometryPosition) || !IsValidRange(config.CameraEye) ||
			!IsValidRange(config.CameraLookAt) || !IsValidRange(config.LightPosition) ||
			!IsValidRange(config.GeometryScale, 0.001f, 100000.0f) ||
			!IsValidRange(config.CameraVerticalFovRadians, 0.1f, 3.0f) ||
			!IsValidRange(config.LightIntensity, 0.0f, 1000000.0f) ||
			!IsValidRange(config.LightAttenuationRadius, 0.001f, 100000.0f) ||
			!IsValidRange(config.LightSourceRadius, 0.0f, 50000.0f) ||
			!IsValidRange(config.RectangleExtent, 0.0f, 50000.0f) ||
			!IsValidRange(config.DirectionalAngularRadius, 0.0f, 0.5f) ||
			!IsValidRange(config.ShadowSoftness, 0.0f, 4.0f)) {
	
			return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::BoundsViolation, "one or more randomization ranges are invalid");
		}

		if (!IsFinite(config.CameraNear) || !IsFinite(config.CameraFar) ||
			config.CameraNear <= 0.0f || config.CameraFar <= config.CameraNear) {
			return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::BoundsViolation, "camera near/far planes are invalid");
		}
	
		for (const auto& scene : SplitSceneAssets(config.SceneAsset)) {
			if (scene.rfind("procedural:", 0u) != 0u && scene.rfind("mixed:", 0u) != 0u) {
				return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::FormatMismatch, "capture requires procedural: or mixed: stable receivers");
			}
		}

		const auto maximumAxisDistance = [](float lightMinimum, float lightMaximum, float anchorMinimum, float anchorMaximum) {
			return std::max(std::abs(lightMinimum - anchorMaximum), std::abs(lightMaximum - anchorMinimum));
		};
		const auto dx = maximumAxisDistance(config.LightPosition.Minimum.X, config.LightPosition.Maximum.X, config.CameraLookAt.Minimum.X, config.CameraLookAt.Maximum.X);
		const auto dy = maximumAxisDistance(config.LightPosition.Minimum.Y, config.LightPosition.Maximum.Y, 0.0f, 0.0f);
		const auto dz = maximumAxisDistance(config.LightPosition.Minimum.Z, config.LightPosition.Maximum.Z,config.CameraLookAt.Minimum.Z, config.CameraLookAt.Maximum.Z);
		const auto maximumCoverageDistance = std::sqrt(dx * dx + dy * dy + dz * dz);
	
		if (config.LightAttenuationRadius.Minimum < maximumCoverageDistance) {
			return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::BoundsViolation, "minimum light attenuation cannot reach every deterministic floor coverage anchor");
		}
	
		if ((config.LightTypeMask & 0x01u) == 0u || config.LightCount.Minimum < 2u) {
			return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::BoundsViolation, "capture requires point support and at least two lights for guaranteed coverage plus type variation");
		}
	
		if (config.SampleCount < 2u || (config.SampleCount & 1u) != 0u) {
			return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::BoundsViolation, "capture requires a positive even sample count for complete two-frame camera sequences");
		}
	
		const auto& atlas = config.Atlas;
	
		if (atlas.Version != CAPTURE_ATLAS_METADATA_VERSION || atlas.Width == 0u || atlas.Height == 0u ||
			atlas.TileSize == 0u || atlas.HardInputEncoding != CAPTURE_HARD_INPUT_ENCODING_R16_FLOAT ||
			!IsFinite(atlas.FilterRadius) || !IsFinite(atlas.DepthRejectionSharpness) ||
			!IsFinite(atlas.NormalRejectionSharpness) || !IsFinite(atlas.BlackLevel) ||
			!IsFinite(atlas.ShadowContrast) || !IsFinite(atlas.NoiseScale)) {
	
			return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::LayoutMismatch, "atlas metadata is invalid or not the required R16 hard-input layout");
		}
	
		return {};
	}

	NSCaptureResult CaptureConfiguration::Load(const std::filesystem::path& path, NSCaptureConfig& config) {
		std::ifstream stream(path);
		if (!stream) return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::IoFailure, "failed to open capture config");
	
		config = {};
		std::map<std::string, std::string> values;
		std::string line;
		std::uint32_t lineNumber = 0u;
	
		while (std::getline(stream, line)) {
			++lineNumber;
			line = Trim(line);
	
			if (line.empty() || line.front() == '#' || line.front() == ';') continue;
		
			const auto separator = line.find('=');
		
			if (separator == std::string::npos) {
				return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::InvalidArgument, "capture config line " + std::to_string(lineNumber) + " lacks '='");
			}
		
			auto key = Trim(line.substr(0u, separator));
			auto value = Trim(line.substr(separator + 1u));
		
			if (key.empty() || value.empty() || !values.emplace(key, value).second) {
				return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::InvalidArgument, "capture config has an empty or duplicate key at line " + std::to_string(lineNumber));
			}
		}

		if (!stream.eof()) return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::IoFailure, "failed while reading capture config");

		auto take = [&values](const char* key, auto& output, auto parser, bool required = false) -> bool {
			const auto found = values.find(key);
	
			if (found == values.end()) return !required;
		
			if (!parser(found->second, output)) return false;
		
			values.erase(found);
			return true;
		};

		auto parseString = [](std::string_view input, std::string& output) { output.assign(input); return !output.empty(); };
		auto parseU32 = [](std::string_view input, std::uint32_t& output) { return ParseUnsigned(input, output); };
		auto parseU64 = [](std::string_view input, std::uint64_t& output) { return ParseUnsigned(input, output); };
		auto parseF32 = [](std::string_view input, float& output) { return ParseFloat(input, output); };
		auto parse = [](std::string_view input, NSCaptureFloat3& output) { return ParseFloat3(input, output); };

		if (!take("version", config.Version, parseU32, true) ||
			!take("dataset_id", config.DatasetId, parseU64, true) ||
			!take("base_seed", config.BaseSeed, parseU32, true) ||
			!take("sample_count", config.SampleCount, parseU32, true) ||
			!take("generation_base", config.GenerationBase, parseU64, true) ||
			!take("scene_asset", config.SceneAsset, parseString, true) ||
			!take("output_width", config.OutputWidth, parseU32, true) ||
			!take("output_height", config.OutputHeight, parseU32, true) ||
			!take("target_spp", config.TargetSamplesPerPixel, parseU32) ||
			!take("geometry_count_min", config.GeometryCount.Minimum, parseU32) ||
			!take("geometry_count_max", config.GeometryCount.Maximum, parseU32) ||
			!take("geometry_type_mask", config.GeometryTypeMask, parseU32) ||
			!take("geometry_position_min", config.GeometryPosition.Minimum, parse) ||
			!take("geometry_position_max", config.GeometryPosition.Maximum, parse) ||
			!take("geometry_scale_min", config.GeometryScale.Minimum, parseF32) ||
			!take("geometry_scale_max", config.GeometryScale.Maximum, parseF32) ||
			!take("camera_eye_min", config.CameraEye.Minimum, parse) ||
			!take("camera_eye_max", config.CameraEye.Maximum, parse) ||
			!take("camera_look_at_min", config.CameraLookAt.Minimum, parse) ||
			!take("camera_look_at_max", config.CameraLookAt.Maximum, parse) ||
			!take("camera_near", config.CameraNear, parseF32) ||
			!take("camera_far", config.CameraFar, parseF32) ||
			!take("camera_fov_y_min", config.CameraVerticalFovRadians.Minimum, parseF32) ||
			!take("camera_fov_y_max", config.CameraVerticalFovRadians.Maximum, parseF32) ||
			!take("light_count_min", config.LightCount.Minimum, parseU32) ||
			!take("light_count_max", config.LightCount.Maximum, parseU32) ||
			!take("light_type_mask", config.LightTypeMask, parseU32) ||
			!take("light_position_min", config.LightPosition.Minimum, parse) ||
			!take("light_position_max", config.LightPosition.Maximum, parse) ||
			!take("light_intensity_min", config.LightIntensity.Minimum, parseF32) ||
			!take("light_intensity_max", config.LightIntensity.Maximum, parseF32) ||
			!take("light_attenuation_min", config.LightAttenuationRadius.Minimum, parseF32) ||
			!take("light_attenuation_max", config.LightAttenuationRadius.Maximum, parseF32) ||
			!take("light_source_radius_min", config.LightSourceRadius.Minimum, parseF32) ||
			!take("light_source_radius_max", config.LightSourceRadius.Maximum, parseF32) ||
			!take("rectangle_extent_min", config.RectangleExtent.Minimum, parseF32) ||
			!take("rectangle_extent_max", config.RectangleExtent.Maximum, parseF32) ||
			!take("directional_angular_radius_min", config.DirectionalAngularRadius.Minimum, parseF32) ||
			!take("directional_angular_radius_max", config.DirectionalAngularRadius.Maximum, parseF32) ||
			!take("shadow_softness_min", config.ShadowSoftness.Minimum, parseF32) ||
			!take("shadow_softness_max", config.ShadowSoftness.Maximum, parseF32) ||
			!take("atlas_width", config.Atlas.Width, parseU32) ||
			!take("atlas_height", config.Atlas.Height, parseU32) ||
			!take("atlas_tile_size", config.Atlas.TileSize, parseU32) ||
			!take("atlas_filter_mode", config.Atlas.FilterMode, parseU32) ||
			!take("atlas_pcf_kernel_size", config.Atlas.PCFKernelSize, parseU32) ||
			!take("atlas_filter_radius", config.Atlas.FilterRadius, parseF32) ||
			!take("atlas_depth_rejection", config.Atlas.DepthRejectionSharpness, parseF32) ||
			!take("atlas_normal_rejection", config.Atlas.NormalRejectionSharpness, parseF32) ||
			!take("atlas_black_level", config.Atlas.BlackLevel, parseF32) ||
			!take("atlas_shadow_contrast", config.Atlas.ShadowContrast, parseF32) ||
			!take("atlas_noise_scale", config.Atlas.NoiseScale, parseF32)) {
		
			return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::InvalidArgument, "capture config contains a malformed value or misses a required key");
	
		}
	
		if (!values.empty()) {
			return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::InvalidArgument, "unknown capture config key: " + values.begin()->first);
		}
		return Validate(config);
	}

	std::uint64_t CaptureConfiguration::ComputeHash(const NSCaptureConfig& config) {
		auto hash = FNV_OFFSET;
		auto add = [&hash](const auto& value) { hash = HashValue(hash, value); };
	
		add(config.Version); add(config.DatasetId); add(config.BaseSeed); add(config.SampleCount); add(config.GenerationBase);
	
		hash = HashBytes(hash, config.SceneAsset.data(), config.SceneAsset.size());
	
		add(config.OutputWidth); 
		add(config.OutputHeight); 
		add(config.TargetSamplesPerPixel); 
		add(config.GeometryCount); 
		add(config.GeometryTypeMask); 
		add(config.GeometryPosition);
		add(config.GeometryScale); 
		add(config.CameraEye); 
		add(config.CameraLookAt); 
		add(config.CameraNear); 
		add(config.CameraFar);
		add(config.CameraVerticalFovRadians); 
		add(config.LightCount); 
		add(config.LightTypeMask); 
		add(config.LightPosition); 
		add(config.LightIntensity);
		add(config.LightAttenuationRadius); 
		add(config.LightSourceRadius); 
		add(config.RectangleExtent);
		add(config.DirectionalAngularRadius); 
		add(config.ShadowSoftness); 
		add(config.Atlas);

		return hash == 0u ? 1u : hash;
	}

	NSCaptureResult CaptureConfiguration::BuildPlan(const NSCaptureConfig& config, std::vector<NSCaptureSamplePlan>& plan) {
		plan.clear();
		auto result = Validate(config);
		if (!result.Succeeded()) return result;

		const auto configHash = ComputeHash(config);
		const auto sceneAssets = SplitSceneAssets(config.SceneAsset);
	
		plan.reserve(config.SampleCount);
	
		for (std::uint32_t sampleIndex = 0u; sampleIndex < config.SampleCount; ++sampleIndex) {
			NSCaptureSamplePlan sample{};
		
			const auto descriptorIndex = sampleIndex / 2u;
			sample.SampleIndex = sampleIndex;
			sample.SampleId = MakeSampleId(config.DatasetId, configHash, sampleIndex);
			sample.GenerationId = config.GenerationBase + sampleIndex;
			sample.SampleSeed = PcgHash(config.BaseSeed ^ descriptorIndex ^ 0x4e53'4331u);
			sample.RelativePath = MakeSamplePath(sampleIndex);
			sample.SceneAsset = sceneAssets[RandomBits(sample.SampleSeed, 6u, 0u, 0u) % sceneAssets.size()];
		
			auto cameraEye = RandomRange(sample.SampleSeed, 5u, 0u, 0u, config.CameraEye);
			auto cameraLookAt = RandomRange(sample.SampleSeed, 5u, 0u, 3u, config.CameraLookAt);
		
			const NSCaptureFloat3 coverageAnchor{ cameraLookAt.X, 0.0f, cameraLookAt.Z };
			const auto cameraOffset = (sampleIndex & 1u) == 0u ? -0.04f : 0.04f;
		
			cameraEye.X += cameraOffset;
			cameraLookAt.X += cameraOffset;

			const auto geometryCount = RandomCount(sample.SampleSeed, 0u, config.GeometryCount);
			sample.Geometry.resize(geometryCount);
			for (std::uint32_t index = 0u; index < geometryCount; ++index) {
				auto& geometry = sample.Geometry[index];
			
				geometry.Kind = 1u + SelectMaskedType(RandomBits(sample.SampleSeed, 1u, index, 0u), config.GeometryTypeMask, 9u);
				geometry.InstanceIndex = index;
				geometry.Position = RandomRange(sample.SampleSeed, 1u, index, 1u, config.GeometryPosition);
				geometry.RotationRadians = {
					RandomUnit(sample.SampleSeed, 1u, index, 4u) * 2.0f * PI,
					RandomUnit(sample.SampleSeed, 1u, index, 5u) * 2.0f * PI,
					RandomUnit(sample.SampleSeed, 1u, index, 6u) * 2.0f * PI
				};
			
				const auto scale = RandomRange(sample.SampleSeed, 1u, index, 7u, config.GeometryScale);
				geometry.Scale = { scale, scale, scale };
			
				switch (static_cast<NSCaptureGeometryKind>(geometry.Kind)) {
				case NSCaptureGeometryKind::Plane:
					geometry.Scale = { scale * 2.5f, scale, scale * 2.5f };
					break;
				case NSCaptureGeometryKind::Cylinder:
					geometry.Scale = { scale, scale * 1.75f, scale };
					break;
				case NSCaptureGeometryKind::Wall:
					geometry.Scale = { scale * 3.0f, scale * 2.0f, std::max(0.03f, scale * 0.08f) };
					break;
				case NSCaptureGeometryKind::StairStep:
					geometry.Scale = { scale * 1.5f, std::max(0.1f, scale * 0.35f), scale };
					geometry.Position.Y += static_cast<float>(index % 4u) * geometry.Scale.Y * 1.5f;
					break;
				case NSCaptureGeometryKind::ThinBox:
					geometry.Scale = { std::max(0.02f, scale * 0.04f), scale * 2.0f, scale };
					break;
				case NSCaptureGeometryKind::IntersectingBox:
					geometry.Scale = { scale, scale * 0.6f, scale * 1.8f };
					if (index > 0u) {
						geometry.Position = sample.Geometry[index - 1u].Position;
						geometry.Position.X += scale * 0.45f;
						geometry.RotationRadians.Y += PI * 0.5f;
					}
					break;
				default:
					break;
				}

				geometry.MaterialSeed = RandomBits(sample.SampleSeed, 1u, index, 8u);
			}

			const auto lightCount = RandomCount(sample.SampleSeed, 2u, config.LightCount);
			sample.Lights.resize(lightCount);
		
			for (std::uint32_t index = 0u; index < lightCount; ++index) {
				auto& light = sample.Lights[index];
				light.LightIndex = index;
				light.LightType = index == 0u ? 0u : SelectMaskedType(RandomBits(sample.SampleSeed, 3u, index, 0u), config.LightTypeMask, 4u);
				light.Position = RandomRange(sample.SampleSeed, 3u, index, 1u, config.LightPosition);
				light.Direction = RandomUnitDirection(sample.SampleSeed, index);
			
				if (light.LightType == 1u) light.Direction = DirectionFromTo(light.Position, coverageAnchor);

				light.Color = {
					0.2f + 0.8f * RandomUnit(sample.SampleSeed, 3u, index, 4u),
					0.2f + 0.8f * RandomUnit(sample.SampleSeed, 3u, index, 5u),
					0.2f + 0.8f * RandomUnit(sample.SampleSeed, 3u, index, 6u)
				};

				light.Intensity = RandomRange(sample.SampleSeed, 3u, index, 7u, config.LightIntensity);
				light.AttenuationRadius = RandomRange(sample.SampleSeed, 3u, index, 8u, config.LightAttenuationRadius);
				light.SourceRadius = RandomRange(sample.SampleSeed, 3u, index, 9u, config.LightSourceRadius);
				light.InnerConeAngle = 0.1f + 0.35f * RandomUnit(sample.SampleSeed, 3u, index, 10u);
				light.OuterConeAngle = light.InnerConeAngle + (1.2f - light.InnerConeAngle) * RandomUnit(sample.SampleSeed, 3u, index, 11u);
				light.RectangleSize = {
					RandomRange(sample.SampleSeed, 3u, index, 12u, config.RectangleExtent),
					RandomRange(sample.SampleSeed, 3u, index, 13u, config.RectangleExtent)
				};

				light.RotationRadians = {
					RandomUnit(sample.SampleSeed, 3u, index, 14u) * 2.0f * PI,
					RandomUnit(sample.SampleSeed, 3u, index, 15u) * 2.0f * PI,
					RandomUnit(sample.SampleSeed, 3u, index, 16u) * 2.0f * PI
				};

				light.DirectionalAngularRadius = RandomRange(sample.SampleSeed, 3u, index, 17u, config.DirectionalAngularRadius);
				light.ShadowSoftness = RandomRange(sample.SampleSeed, 3u, index, 18u, config.ShadowSoftness);
				light.ParameterSeed = RandomBits(sample.SampleSeed, 3u, index, 19u);
			}

			auto& metadata = sample.Metadata;
			metadata.ConfigVersion = config.Version;
			metadata.RandomizationVersion = CAPTURE_RANDOMIZATION_ALGORITHM_VERSION;
			metadata.SampleId = sample.SampleId;
			metadata.GenerationId = sample.GenerationId;
			metadata.DatasetId = config.DatasetId;
			metadata.ConfigHash = configHash;
			metadata.BaseSeed = config.BaseSeed;
			metadata.SampleSeed = sample.SampleSeed;
			metadata.OutputWidth = config.OutputWidth;
			metadata.OutputHeight = config.OutputHeight;
			metadata.FeatureLayoutVersion = FDW_SD::Features::FEATURE_LAYOUT_VERSION;
			metadata.PixelFeatureChannelCount = FDW_SD::Features::PIXEL_CHANNEL_COUNT;
			metadata.LightFeatureChannelCount = FDW_SD::Features::LIGHT_CHANNEL_COUNT;
		
			metadata.Camera.Eye = cameraEye;
			metadata.Camera.LookAt = cameraLookAt;
			metadata.Camera.NearPlane = config.CameraNear;
			metadata.Camera.FarPlane = config.CameraFar;
			metadata.Camera.VerticalFovRadians = RandomRange(sample.SampleSeed, 5u, 0u, 6u, config.CameraVerticalFovRadians);
			metadata.Camera.AspectRatio = static_cast<float>(config.OutputWidth) / config.OutputHeight;
			metadata.Camera.JitterPixels = {};
		
			metadata.Atlas = config.Atlas;
			metadata.Atlas.ShadowMetadataCount = lightCount;
		
			sample.MetadataBytes = SerializeMetadata(metadata, sample.SceneAsset, sample.Geometry, sample.Lights);
			plan.push_back(std::move(sample));
		}
		return {};
	}

	NSCaptureResult CaptureConfiguration::ValidateMetadata(std::span<const std::byte> bytes, NSCaptureMetadataHeader* outputHeader) {
		if (bytes.size() < sizeof(NSCaptureMetadataHeader)) {
			return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::Truncated, "capture metadata header is truncated");
		}
	
		NSCaptureMetadataHeader header{};
		std::memcpy(&header, bytes.data(), sizeof(header));
	
		const auto validVersionPair = header.ConfigVersion == CAPTURE_CONFIG_FORMAT_VERSION &&
			header.RandomizationVersion == CAPTURE_RANDOMIZATION_ALGORITHM_VERSION;
	
		if (header.Magic != CAPTURE_METADATA_MAGIC || header.Version != CAPTURE_METADATA_VERSION || !validVersionPair) {
			return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::VersionMismatch, "capture metadata identity/version mismatch");
		}
	
		if (header.HeaderByteSize != sizeof(header) || header.FileByteSize != bytes.size() ||
			header.Flags != 0u || header.SampleId == 0u || header.GenerationId == 0u || header.DatasetId == 0u ||
			header.ConfigHash == 0u || header.CompletionMarker != CAPTURE_COMPLETION_MARKER) {

			return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::LayoutMismatch, "capture metadata header fields are invalid");
		}
	
		if (header.FeatureLayoutVersion != FDW_SD::Features::FEATURE_LAYOUT_VERSION ||
			header.PixelFeatureChannelCount != FDW_SD::Features::PIXEL_CHANNEL_COUNT ||
			header.LightFeatureChannelCount != FDW_SD::Features::LIGHT_CHANNEL_COUNT ||
			header.SeedAlgorithmVersion != CAPTURE_SEED_ALGORITHM_VERSION) {

			return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::FormatMismatch, "capture metadata does not declare the active feature and seed layouts");
		}

		const auto geometryBytes = static_cast<std::uint64_t>(header.GeometryCount) * header.GeometryStride;
		const auto lightBytes = static_cast<std::uint64_t>(header.LightCount) * header.LightStride;
	
		const auto expectedGeometryOffset64 = (static_cast<std::uint64_t>(sizeof(header)) + header.ScenePathByteSize + 15u) & ~15ull;
		const auto expectedLightOffset64 = (expectedGeometryOffset64 + geometryBytes + 15u) & ~15ull;
	
		if (header.ScenePathByteSize == 0u || header.ScenePathOffset != sizeof(header) ||
			header.GeometryStride != sizeof(NSCaptureGeometryMetadata) ||
			header.LightStride != sizeof(NSCaptureLightMetadata) ||
			header.GeometryCount > CAPTURE_MAX_GEOMETRY_COUNT ||
			header.LightCount == 0u || header.LightCount > CAPTURE_MAX_LIGHT_COUNT ||
			expectedGeometryOffset64 > std::numeric_limits<std::uint32_t>::max() ||
			expectedLightOffset64 > std::numeric_limits<std::uint32_t>::max() ||
			header.GeometryOffset != expectedGeometryOffset64 ||
			header.LightOffset != expectedLightOffset64 ||
			static_cast<std::uint64_t>(header.LightOffset) + lightBytes != bytes.size()) {

			return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::BoundsViolation, "capture metadata record ranges are invalid");
		}

		if (header.OutputWidth == 0u || header.OutputHeight == 0u ||
			header.Atlas.Version != CAPTURE_ATLAS_METADATA_VERSION ||
			header.Atlas.HardInputEncoding != CAPTURE_HARD_INPUT_ENCODING_R16_FLOAT ||
			header.Atlas.ShadowMetadataCount != header.LightCount ||
			header.Padding0 != 0u || header.Padding1 != 0u || header.Padding2 != 0u || header.Padding3 != 0u) {

			return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::LayoutMismatch, "capture output/atlas/padding metadata is invalid");
		}

		if (!IsFinite(header.Camera.Eye) || !IsFinite(header.Camera.LookAt) || !IsFinite(header.Camera.Up) ||
			!IsFinite(header.Camera.NearPlane) || !IsFinite(header.Camera.FarPlane) ||
			!IsFinite(header.Camera.VerticalFovRadians) || !IsFinite(header.Camera.AspectRatio) ||
			header.Camera.NearPlane <= 0.0f || header.Camera.FarPlane <= header.Camera.NearPlane ||
			header.Camera.VerticalFovRadians <= 0.0f || header.Camera.AspectRatio <= 0.0f ||
			header.Camera.JitterPixels.X != 0.0f || header.Camera.JitterPixels.Y != 0.0f ||
			header.Camera.Padding0 != 0u) {

			return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::LayoutMismatch, "capture camera metadata is invalid");

		}

		for (std::uint32_t index = 0u; index < header.GeometryCount; ++index) {
			NSCaptureGeometryMetadata geometry{};
			std::memcpy(&geometry, bytes.data() + header.GeometryOffset + index * sizeof(geometry), sizeof(geometry));
		
			if (geometry.Kind < static_cast<std::uint32_t>(NSCaptureGeometryKind::Plane) ||
				geometry.Kind > static_cast<std::uint32_t>(NSCaptureGeometryKind::IntersectingBox) ||
				geometry.InstanceIndex != index || !IsFinite(geometry.Position) || !IsFinite(geometry.RotationRadians) ||
				!IsFinite(geometry.Scale) || geometry.Scale.X <= 0.0f || geometry.Scale.Y <= 0.0f || geometry.Scale.Z <= 0.0f ||
				geometry.Padding0 != 0u || geometry.Padding1 != 0u || geometry.Padding2 != 0u) {

				return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::LayoutMismatch, "capture geometry metadata is invalid");
			}
		}
	
		for (std::uint32_t index = 0u; index < header.LightCount; ++index) {
			NSCaptureLightMetadata light{};
			std::memcpy(&light, bytes.data() + header.LightOffset + index * sizeof(light), sizeof(light));
		
			if (light.LightIndex != index || light.LightType > 3u || !IsFinite(light.Position) ||
				!IsFinite(light.Direction) || !IsFinite(light.Color) || !IsFinite(light.Intensity) ||
				!IsFinite(light.AttenuationRadius) || !IsFinite(light.SourceRadius) ||
				!IsFinite(light.InnerConeAngle) || !IsFinite(light.OuterConeAngle) ||
				!IsFinite(light.RectangleSize.X) || !IsFinite(light.RectangleSize.Y) ||
				!IsFinite(light.RotationRadians) || !IsFinite(light.DirectionalAngularRadius) ||
				!IsFinite(light.ShadowSoftness) || light.AttenuationRadius <= 0.0f || light.SourceRadius < 0.0f ||
				light.DirectionalAngularRadius < 0.0f || light.DirectionalAngularRadius > 0.5f ||
				light.ShadowSoftness < 0.0f || light.ShadowSoftness > 4.0f ||
				light.ShadowParameterFlags != CAPTURE_LIGHT_HAS_SHADOW_PARAMETERS ||
				light.Padding0 != 0u || light.Padding1 != 0u || light.Padding2 != 0u) {

				return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::LayoutMismatch, "capture light metadata is invalid");
			}
		}

		if (outputHeader) *outputHeader = header;

		return {};
	}

	// Called only after ValidateInputBlocksCore has checked the feature block layouts.
	NSCaptureResult CaptureConfiguration::ValidatePixelFeatures(const NSCaptureOwnedBlock& block, std::uint32_t workCount) {
		const auto* pixelValues = reinterpret_cast<const std::uint16_t*>(block.Bytes.data());
		for (std::uint32_t batch = 0u; batch < workCount; ++batch) {

			for (std::uint32_t pixel = 0u; pixel < FDW_SD::Features::PIXELS_PER_PATCH; ++pixel) {
				const auto validityOffset = (static_cast<std::uint64_t>(batch) * FDW_SD::Features::PIXEL_CHANNEL_COUNT + static_cast<std::uint32_t>(FDW_SD::Features::NSPixelFeatureChannel::ReceiverValid)) * FDW_SD::Features::PIXELS_PER_PATCH + pixel;

				const auto validity = DecodeFloat16(pixelValues[validityOffset]);
				if (validity != 0.0f && validity != 1.0f) {
					return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::LayoutMismatch, "pixel receiver-valid channel is not binary FP16");
				}

				for (std::uint32_t channel = 0u; channel < FDW_SD::Features::PIXEL_CHANNEL_COUNT; ++channel) {
					const auto offset = (static_cast<std::uint64_t>(batch) *FDW_SD::Features::PIXEL_CHANNEL_COUNT + channel) * FDW_SD::Features::PIXELS_PER_PATCH + pixel;
					const auto value = DecodeFloat16(pixelValues[offset]);
					const auto isUnsignedChannel = channel == 0u || channel == 1u || channel == 8u || channel == 9u;

					if (!IsFinite(value) || value < (isUnsignedChannel ? 0.0f : -1.0f) || value > 1.0f ||
						(validity == 0.0f && value != 0.0f)) {

						return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::LayoutMismatch, "pixel feature value violates the active bounds or padding");
					}
				}
			}
		}
		return {};
	}

	NSCaptureResult CaptureConfiguration::ValidateLightFeatures(const NSCaptureOwnedBlock& block, std::uint32_t workCount) {
		const auto* lightValues = reinterpret_cast<const std::uint16_t*>(block.Bytes.data());

		for (std::uint32_t batch = 0u; batch < workCount; ++batch) {

			std::uint32_t activeTypes = 0u;
			for (std::uint32_t channel = 0u; channel < FDW_SD::Features::LIGHT_CHANNEL_COUNT; ++channel) {

				const auto value = DecodeFloat16(lightValues[static_cast<std::uint64_t>(batch) * FDW_SD::Features::LIGHT_CHANNEL_COUNT + channel]);
				const auto isQuaternionChannel = channel >= 8u && channel <= 11u;

				if (!IsFinite(value) || value < (isQuaternionChannel ? -1.0f : 0.0f) || value > 1.0f) {
					return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::LayoutMismatch, "light feature value violates the active bounds");
				}

				if (channel < 4u) {
					if (value != 0.0f && value != 1.0f) {
						return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::LayoutMismatch, "light type feature is not binary FP16");
					}

					activeTypes += value == 1.0f ? 1u : 0u;
				}
			}

			if (activeTypes != 1u) return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::LayoutMismatch, "light feature must have one active type");
		}
		return {};
	}

	NSCaptureResult CaptureConfiguration::ValidateInputBlocksCore(const NSCaptureSamplePlan& sample,std::span<const NSCaptureOwnedBlock> blocks, bool allowGroundTruthBlocks) {
	
		if (blocks.empty()) return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::InvalidArgument, "capture input blocks are empty");
	
		std::map<std::uint32_t, std::uint32_t> occurrences;
	
		for (const auto& block : blocks) {
			if (++occurrences[static_cast<std::uint32_t>(block.Kind)] != 1u) {
				return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::DuplicateBlock, "capture input block kinds must be unique");
			}
		
			if (!allowGroundTruthBlocks && (block.Kind == NSNeuralShadowDataset::NSSampleBlockKind::RTTargetVisibility ||
				block.Kind == NSNeuralShadowDataset::NSSampleBlockKind::TargetValidity)) {
				return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::InvalidArgument, "capture metadata cannot publish RT target/validity blocks");
			}
		
			if (block.Kind == NSNeuralShadowDataset::NSSampleBlockKind::CaptureMetadata ||
				block.Kind == NSNeuralShadowDataset::NSSampleBlockKind::RTTargetMetadata) {
				return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::DuplicateBlock, "capture metadata is owned by the deterministic plan");
			}
		}
	
		const auto* workHeaderBlock = FindBlock(blocks, NSNeuralShadowDataset::NSSampleBlockKind::WorkHeader);
		const auto* workItemsBlock = FindBlock(blocks, NSNeuralShadowDataset::NSSampleBlockKind::WorkItems);
	
		const auto* pixelBlock = FindBlock(blocks, NSNeuralShadowDataset::NSSampleBlockKind::PixelFeatures);
		const auto* lightBlock = FindBlock(blocks, NSNeuralShadowDataset::NSSampleBlockKind::LightFeatures);
		const auto* hardBlock = FindBlock(blocks, NSNeuralShadowDataset::NSSampleBlockKind::HardInputVisibility);
	
		if (!workHeaderBlock || !workItemsBlock || !pixelBlock || !lightBlock || !hardBlock) {
			return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::Incomplete, "capture requires work header/items, pixel/light features, and hard input");
		}

		FDW_SD::Work::NSWorkDispatchHeader workHeader{};
	
		if (workHeaderBlock->Version != FDW_SD::Work::DATA_LAYOUT_VERSION ||
			workHeaderBlock->ElementCount != 1u || !ReadStruct(workHeaderBlock->Bytes, workHeader) ||
			!FDW_SD::Work::IsDatasetPublishable(workHeader) ||
			FDW_SD::Work::GetGeneration(workHeader) != sample.GenerationId ||
			workHeader.ScreenWidth != sample.Metadata.OutputWidth || workHeader.ScreenHeight != sample.Metadata.OutputHeight) {

			return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::SampleMismatch, "work header is incomplete or does not match the planned sample");
		}

		const auto workCount = static_cast<std::uint64_t>(workHeader.WorkItemCount);
		const auto outputCount = static_cast<std::uint64_t>(workHeader.OutputValueCount);
	
		if (workCount == 0u) return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::Incomplete, "capture sample contains no active shadow work");

		const auto expectedWorkBytes = workCount * sizeof(FDW_SD::Work::NSWorkItem);
		const auto expectedPixelElements = outputCount * FDW_SD::Features::PIXEL_CHANNEL_COUNT;
		const auto expectedLightElements = workCount * FDW_SD::Features::LIGHT_CHANNEL_COUNT;
	
		if (!workItemsBlock->HasLayout(FDW_SD::Work::DATA_LAYOUT_VERSION, workCount, expectedWorkBytes) ||
			!pixelBlock->HasLayout(FDW_SD::Features::FEATURE_LAYOUT_VERSION, expectedPixelElements, expectedPixelElements * sizeof(std::uint16_t)) ||
			!lightBlock->HasLayout(FDW_SD::Features::FEATURE_LAYOUT_VERSION, expectedLightElements, expectedLightElements * sizeof(std::uint16_t)) ||
			!hardBlock->HasLayout(1u, outputCount, outputCount * sizeof(std::uint16_t))) {

			return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::LayoutMismatch, "capture tensor bytes/elements do not match the active feature shapes");
		}

		for (std::uint32_t index = 0u; index < workHeader.WorkItemCount; ++index) {
			FDW_SD::Work::NSWorkItem item{};
			std::memcpy(&item, workItemsBlock->Bytes.data() + index * sizeof(item), sizeof(item));
		
			if (item.ScreenTileIndex >= workHeader.PatchCountX * workHeader.PatchCountY ||
				item.OutputOffset != FDW_SD::Work::ComputeOutputOffset(index) ||
				item.LightIndex >= sample.Metadata.LightCount || item.ShadowMetaIndex >= sample.Metadata.LightCount) {

				return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::SampleMismatch, "work identity is not canonical or exceeds planned lights");
			}
		}

		auto result = ValidatePixelFeatures(*pixelBlock, workHeader.WorkItemCount);
		if (!result.Succeeded()) return result;

		result = ValidateLightFeatures(*lightBlock, workHeader.WorkItemCount);
		if (!result.Succeeded()) return result;

		const auto* hardValues = reinterpret_cast<const std::uint16_t*>(hardBlock->Bytes.data());
		for (std::uint64_t index = 0u; index < outputCount; ++index) {
	
			const auto value = DecodeFloat16(hardValues[index]);
		
			if (!IsFinite(value) || value < 0.0f || value > 1.0f) {
				return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::LayoutMismatch, "hard-input visibility is not finite normalized R16");
			}
		}

		return ValidateMetadata(sample.MetadataBytes);
	}

	NSCaptureResult CaptureConfiguration::ValidateInputBlocks(const NSCaptureSamplePlan& sample, std::span<const NSCaptureOwnedBlock> blocks) {
		return ValidateInputBlocksCore(sample, blocks, false);
	}

	NSCaptureResult CaptureConfiguration::ValidateGroundTruthInputBlocks(const NSCaptureSamplePlan& sample, std::span<const NSCaptureOwnedBlock> blocks) {
		return ValidateInputBlocksCore(sample, blocks, true);
	}

	NSCaptureResult CaptureConfiguration::ExecutePlan(const NSCaptureConfig& config, const std::filesystem::path& datasetRoot, const NSCaptureInputProvider& inputProvider, NSCaptureRunStats& stats) {
		stats = {};
		if (datasetRoot.empty() || !inputProvider) {
			return Failure(NSNeuralShadowDataset::NSDatasetErrorCode::InvalidArgument, "dataset root and capture input provider are required");
		}
	
		std::vector<NSCaptureSamplePlan> plan;
	
		auto result = BuildPlan(config, plan);
		if (!result.Succeeded()) return result;
	
		stats.Planned = static_cast<std::uint32_t>(plan.size());
		for (const auto& sample : plan) {
			NSNeuralShadowDataset::NSDatasetResult regenerationDetail{};
			const auto regeneration = NSNeuralShadowDataset::DatasetContainer::GetSampleRegenerationStatus(
				datasetRoot,
				config.DatasetId,
				sample.RelativePath,
				sample.SampleId,
				sample.GenerationId,
				regenerationDetail);

			if (regeneration == NSNeuralShadowDataset::NSRegenerationStatus::SkipValid) {
				++stats.Skipped;
				continue;
			}

			if (regeneration == NSNeuralShadowDataset::NSRegenerationStatus::Conflict) {
				return Failure(regenerationDetail.Code, "capture resume conflict: " + regenerationDetail.Message);
			}

			std::vector<NSCaptureOwnedBlock> ownedBlocks;
		
			result = inputProvider(sample, ownedBlocks);
			if (!result.Succeeded()) return result;
		
			result = ValidateInputBlocks(sample, ownedBlocks);
			if (!result.Succeeded()) return result;
		
			ownedBlocks.push_back({NSNeuralShadowDataset::NSSampleBlockKind::CaptureMetadata, CAPTURE_METADATA_VERSION, 1u, sample.MetadataBytes });
		
			std::vector<NSNeuralShadowDataset::NSSampleBlockInput> inputs;
			inputs.reserve(ownedBlocks.size());
			for (const auto& block : ownedBlocks) {
				inputs.push_back({ block.Kind, block.Version, block.ElementCount, block.Bytes });
			}

			const auto publication = NSNeuralShadowDataset::DatasetContainer::PublishDatasetSample(datasetRoot, config.DatasetId, sample.RelativePath, sample.SampleId, sample.GenerationId, inputs);
		
			if (!publication.Succeeded()) return Failure(publication.Code, publication.Message);
		
			if (regeneration == NSNeuralShadowDataset::NSRegenerationStatus::RegenerateInvalid) ++stats.Regenerated;
			else ++stats.Generated;
	
		}
		return {};
	}

}
