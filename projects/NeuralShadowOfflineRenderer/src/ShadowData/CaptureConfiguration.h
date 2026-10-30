#pragma once

#include <Dataset/DatasetContainer.h>
#include <ShadowData/ShadowLightParameters.h>

namespace FDW_SD::Capture {

inline constexpr std::uint32_t CAPTURE_CONFIG_FORMAT_VERSION = 4u;
inline constexpr std::uint32_t CAPTURE_METADATA_VERSION = 1u;
inline constexpr std::uint32_t CAPTURE_RANDOMIZATION_ALGORITHM_VERSION = 5u;
inline constexpr std::uint32_t CAPTURE_SEED_ALGORITHM_VERSION = 1u;
inline constexpr std::uint32_t CAPTURE_METADATA_MAGIC = 0x3143'534eu; // NSC1
inline constexpr std::uint32_t CAPTURE_COMPLETION_MARKER = 0x454e'4f44u; // DONE
inline constexpr std::uint32_t CAPTURE_LIGHT_HAS_SHADOW_PARAMETERS = 1u << 0u;
inline constexpr std::uint32_t CAPTURE_MAX_SAMPLE_COUNT = 1'000'000u;
inline constexpr std::uint32_t CAPTURE_MAX_GEOMETRY_COUNT = 4'096u;
inline constexpr std::uint32_t CAPTURE_MAX_LIGHT_COUNT = 5'000u;
inline constexpr std::uint32_t CAPTURE_ATLAS_METADATA_VERSION = 1u;
inline constexpr std::uint32_t CAPTURE_HARD_INPUT_ENCODING_R16_FLOAT = 1u;
inline constexpr std::uint32_t CAPTURE_MIN_VALID_TARGETS = 64u;

struct NSCaptureFloat2 {
	float X = 0.0f;
	float Y = 0.0f;
};

struct NSCaptureFloat3 {
	float X = 0.0f;
	float Y = 0.0f;
	float Z = 0.0f;
};

struct NSCaptureUIntRange {
	std::uint32_t Minimum = 0u;
	std::uint32_t Maximum = 0u;
};

struct NSCaptureFloatRange {
	float Minimum = 0.0f;
	float Maximum = 0.0f;
};

struct NSCaptureFloat3Range {
	NSCaptureFloat3 Minimum{};
	NSCaptureFloat3 Maximum{};
};

struct NSCaptureCameraMetadata {
	NSCaptureFloat3 Eye{};
	NSCaptureFloat3 LookAt{};
	NSCaptureFloat3 Up{ 0.0f, 1.0f, 0.0f };
	
	float NearPlane = 0.1f;
	float FarPlane = 1000.0f;
	float VerticalFovRadians = 1.0471975512f;
	float AspectRatio = 1.0f;
	
	NSCaptureFloat2 JitterPixels{};
	std::uint32_t Padding0 = 0u;
};

struct NSCaptureAtlasMetadata {
	std::uint32_t Version = CAPTURE_ATLAS_METADATA_VERSION;
	std::uint32_t Width = 8192u;
	std::uint32_t Height = 8192u;
	std::uint32_t TileSize = 128u;
	std::uint32_t FilterMode = 0u;
	std::uint32_t PCFKernelSize = 1u;
	
	float FilterRadius = 0.0f;
	float DepthRejectionSharpness = 0.0f;
	float NormalRejectionSharpness = 0.0f;
	float BlackLevel = 0.0f;
	float ShadowContrast = 1.0f;
	float NoiseScale = 0.0f;
	
	std::uint32_t HardInputEncoding = CAPTURE_HARD_INPUT_ENCODING_R16_FLOAT;
	std::uint32_t ShadowMetadataCount = 0u;
	std::uint32_t Padding0 = 0u;
	std::uint32_t Padding1 = 0u;
};

enum class NSCaptureGeometryKind : std::uint32_t {
	Plane = 1u,
	Cube = 2u,
	Sphere = 3u,
	Cone = 4u,
	Cylinder = 5u,
	Wall = 6u,
	StairStep = 7u,
	ThinBox = 8u,
	IntersectingBox = 9u
};

struct NSCaptureGeometryMetadata {
	std::uint32_t Kind = static_cast<std::uint32_t>(NSCaptureGeometryKind::Cube);
	std::uint32_t InstanceIndex = 0u;
	
	NSCaptureFloat3 Position{};
	NSCaptureFloat3 RotationRadians{};
	NSCaptureFloat3 Scale{ 1.0f, 1.0f, 1.0f };
	
	std::uint32_t MaterialSeed = 0u;
	std::uint32_t Flags = 0u;
	std::uint32_t Padding0 = 0u;
	std::uint32_t Padding1 = 0u;
	std::uint32_t Padding2 = 0u;
};

struct NSCaptureLightMetadata {
	std::uint32_t LightIndex = 0u;
	std::uint32_t LightType = 0u;
	
	NSCaptureFloat3 Position{};
	NSCaptureFloat3 Direction{ 0.0f, -1.0f, 0.0f };
	NSCaptureFloat3 Color{ 1.0f, 1.0f, 1.0f };
	
	float Intensity = 1.0f;
	float AttenuationRadius = 10.0f;
	float SourceRadius = 0.0f;
	float InnerConeAngle = 0.2617993878f;
	float OuterConeAngle = 0.5235987756f;
	
	NSCaptureFloat2 RectangleSize{ 1.0f, 1.0f };
	NSCaptureFloat3 RotationRadians{};
	
	float DirectionalAngularRadius = 0.0f;
	float ShadowSoftness = 1.0f;
	
	std::uint32_t ShadowParameterFlags = CAPTURE_LIGHT_HAS_SHADOW_PARAMETERS;
	std::uint32_t ParameterSeed = 0u;
	std::uint32_t Padding0 = 0u;
	std::uint32_t Padding1 = 0u;
	std::uint32_t Padding2 = 0u;
};

struct NSCaptureMetadataHeader {
	std::uint32_t Magic = CAPTURE_METADATA_MAGIC;
	std::uint32_t Version = CAPTURE_METADATA_VERSION;
	std::uint32_t HeaderByteSize = 0u;
	std::uint32_t FileByteSize = 0u;
	std::uint32_t ConfigVersion = CAPTURE_CONFIG_FORMAT_VERSION;
	std::uint32_t Flags = 0u;
	std::uint64_t SampleId = 0u;
	std::uint64_t GenerationId = 0u;
	std::uint64_t DatasetId = 0u;
	std::uint64_t ConfigHash = 0u;
	std::uint32_t BaseSeed = 0u;
	std::uint32_t SampleSeed = 0u;
	std::uint32_t OutputWidth = 0u;
	std::uint32_t OutputHeight = 0u;
	std::uint32_t ScenePathOffset = 0u;
	std::uint32_t ScenePathByteSize = 0u;
	std::uint32_t GeometryOffset = 0u;
	std::uint32_t GeometryCount = 0u;
	std::uint32_t GeometryStride = 0u;
	std::uint32_t LightOffset = 0u;
	std::uint32_t LightCount = 0u;
	std::uint32_t LightStride = 0u;
	std::uint32_t FeatureLayoutVersion = 0u;
	std::uint32_t PixelFeatureChannelCount = 0u;
	std::uint32_t LightFeatureChannelCount = 0u;
	std::uint32_t SeedAlgorithmVersion = CAPTURE_SEED_ALGORITHM_VERSION;
	std::uint32_t RandomizationVersion = CAPTURE_RANDOMIZATION_ALGORITHM_VERSION;
	std::uint32_t CompletionMarker = CAPTURE_COMPLETION_MARKER;
	
	NSCaptureCameraMetadata Camera{};
	NSCaptureAtlasMetadata Atlas{};
	
	std::uint32_t Padding0 = 0u;
	std::uint32_t Padding1 = 0u;
	std::uint32_t Padding2 = 0u;
	std::uint32_t Padding3 = 0u;
};

struct NSCaptureConfig {
	std::uint32_t Version = CAPTURE_CONFIG_FORMAT_VERSION;
	std::uint64_t DatasetId = 0u;
	std::uint32_t BaseSeed = 0u;
	std::uint32_t SampleCount = 0u;
	std::uint64_t GenerationBase = 1u;
	
	std::string SceneAsset;
	
	std::uint32_t OutputWidth = 1920u;
	std::uint32_t OutputHeight = 1080u;
	std::uint32_t TargetSamplesPerPixel = NS_RT_OFFLINE_FINITE_SAMPLE_COUNT;
	
	NSCaptureUIntRange GeometryCount{ 1u, 8u };
	
	std::uint32_t GeometryTypeMask = 0x01ffu;
	
	NSCaptureFloat3Range GeometryPosition{ { -10.0f, 0.0f, -10.0f }, { 10.0f, 10.0f, 10.0f } };
	NSCaptureFloatRange GeometryScale{ 0.25f, 3.0f };
	NSCaptureFloat3Range CameraEye{ { -12.0f, 2.0f, -12.0f }, { 12.0f, 12.0f, 12.0f } };
	NSCaptureFloat3Range CameraLookAt{ { -2.0f, 0.0f, -2.0f }, { 2.0f, 4.0f, 2.0f } };
	
	float CameraNear = 0.1f;
	float CameraFar = 1000.0f;
	
	NSCaptureFloatRange CameraVerticalFovRadians{ 0.7853981634f, 1.2217304764f };
	NSCaptureUIntRange LightCount{ 1u, 8u };
	
	std::uint32_t LightTypeMask = 0x0fu;
	
	NSCaptureFloat3Range LightPosition{ { -12.0f, 1.0f, -12.0f }, { 12.0f, 15.0f, 12.0f } };
	NSCaptureFloatRange LightIntensity{ 1.0f, 100.0f };
	NSCaptureFloatRange LightAttenuationRadius{ 2.0f, 30.0f };
	NSCaptureFloatRange LightSourceRadius{ 0.0f, 2.0f };
	NSCaptureFloatRange RectangleExtent{ 0.25f, 4.0f };
	NSCaptureFloatRange DirectionalAngularRadius{ 0.0f, 0.1f };
	NSCaptureFloatRange ShadowSoftness{ 0.5f, 2.0f };
	
	NSCaptureAtlasMetadata Atlas{};
};

struct NSCaptureResult {
	NSNeuralShadowDataset::NSDatasetErrorCode Code = NSNeuralShadowDataset::NSDatasetErrorCode::None;
	
	std::string Message;
	
	bool Succeeded() const;
};

struct NSCaptureSamplePlan {
	std::uint32_t SampleIndex = 0u;
	std::uint64_t SampleId = 0u;
	std::uint64_t GenerationId = 0u;
	std::uint32_t SampleSeed = 0u;
	
	std::filesystem::path RelativePath;
	
	std::string SceneAsset;
	
	NSCaptureMetadataHeader Metadata{};
	
	std::vector<NSCaptureGeometryMetadata> Geometry;
	std::vector<NSCaptureLightMetadata> Lights;
	std::vector<std::byte> MetadataBytes;
};

struct NSCaptureOwnedBlock {
	NSNeuralShadowDataset::NSSampleBlockKind Kind =NSNeuralShadowDataset::NSSampleBlockKind::WorkHeader;
	
	std::uint32_t Version = 1u;
	std::uint64_t ElementCount = 0u;
	std::vector<std::byte> Bytes;

	bool HasLayout(std::uint32_t version, std::uint64_t elementCount, std::uint64_t byteCount) const;
};

struct NSCaptureRunStats {
	std::uint32_t Planned = 0u;
	std::uint32_t Generated = 0u;
	std::uint32_t Regenerated = 0u;
	std::uint32_t Skipped = 0u;
};

using NSCaptureInputProvider = std::function<NSCaptureResult(const NSCaptureSamplePlan&, std::vector<NSCaptureOwnedBlock>&)>;

class CaptureConfiguration {
public:
	static NSCaptureResult Load(const std::filesystem::path& path,NSCaptureConfig& config);
	
	static NSCaptureResult Validate(const NSCaptureConfig& config);
	
	static std::uint64_t ComputeHash(const NSCaptureConfig& config);
	
	static NSCaptureResult BuildPlan(const NSCaptureConfig& config,std::vector<NSCaptureSamplePlan>& plan);
	
	static NSCaptureResult ValidateMetadata(std::span<const std::byte> bytes,NSCaptureMetadataHeader* header = nullptr);
	
	static std::uint32_t RequiredValidTargetCount(const NSCaptureSamplePlan& sample);
	
	static NSCaptureResult ValidateInputBlocks(const NSCaptureSamplePlan& sample,std::span<const NSCaptureOwnedBlock> blocks);
	
	static NSCaptureResult ValidateGroundTruthInputBlocks(const NSCaptureSamplePlan& sample,std::span<const NSCaptureOwnedBlock> blocks);
	
	static NSCaptureResult ExecutePlan(const NSCaptureConfig& config,const std::filesystem::path& datasetRoot,const NSCaptureInputProvider& inputProvider,NSCaptureRunStats& stats);

protected:
	inline static constexpr float PI = 3.14159265358979323846f;
	inline static constexpr std::uint64_t FNV_OFFSET = 14695981039346656037ull;
	inline static constexpr std::uint64_t FNV_PRIME = 1099511628211ull;

protected:
	static NSCaptureResult Failure(NSNeuralShadowDataset::NSDatasetErrorCode code, std::string message);
	

protected:
	static bool IsFinite(float value);
	static bool IsFinite(NSCaptureFloat3 value);
	static float DecodeFloat16(std::uint16_t bits);
	
protected:
	static bool IsValidRange(NSCaptureUIntRange range, std::uint32_t maximum);
	static bool IsValidRange(NSCaptureFloatRange range, float minimum, float maximum);
	static bool IsValidRange(NSCaptureFloat3Range range);
	static std::string Trim(std::string value);

protected:
	template<class T>
	static bool ParseUnsigned(std::string_view value, T& result);

protected:
	static bool ParseFloat(std::string_view value, float& result);
	static bool ParseFloat3(std::string_view value, NSCaptureFloat3& result);

	static std::uint32_t PcgHash(std::uint32_t input);
	
	static std::uint32_t RandomBits(std::uint32_t sampleSeed, std::uint32_t domain, std::uint32_t object, std::uint32_t dimension);
	
	static float RandomUnit(std::uint32_t sampleSeed, std::uint32_t domain, std::uint32_t object, std::uint32_t dimension);
	
	static float RandomRange(std::uint32_t seed, std::uint32_t domain, std::uint32_t object,std::uint32_t dimension, NSCaptureFloatRange range);
	
	static NSCaptureFloat3 RandomRange(std::uint32_t seed, std::uint32_t domain, std::uint32_t object,std::uint32_t firstDimension, NSCaptureFloat3Range range);
	
	static std::uint32_t RandomCount(std::uint32_t seed, std::uint32_t domain, NSCaptureUIntRange range);

protected:
	static std::uint32_t SelectMaskedType(std::uint32_t bits, std::uint32_t mask, std::uint32_t typeCount);
	
	static std::vector<std::string> SplitSceneAssets(std::string_view value);
	
	static NSCaptureFloat3 RandomUnitDirection(std::uint32_t seed, std::uint32_t object);
	static NSCaptureFloat3 DirectionFromTo(NSCaptureFloat3 from, NSCaptureFloat3 to);
	
	static std::uint64_t HashBytes(std::uint64_t hash, const void* data, std::size_t size);

protected:
	template<class T>
	static std::uint64_t HashValue(std::uint64_t hash, const T& value);

protected:
	static std::uint64_t MakeSampleId(std::uint64_t datasetId, std::uint64_t configHash, std::uint32_t sampleIndex);
	
	static std::filesystem::path MakeSamplePath(std::uint32_t sampleIndex);
	
	static std::uint32_t Align16(std::uint32_t value);
	
	static std::vector<std::byte> SerializeMetadata(NSCaptureMetadataHeader& header,const std::string& scene,std::span<const NSCaptureGeometryMetadata> geometry,std::span<const NSCaptureLightMetadata> lights);
	
protected:
	template<class T>
	static bool ReadStruct(std::span<const std::byte> bytes, T& value);
	

protected:
	static const NSCaptureOwnedBlock* FindBlock(std::span<const NSCaptureOwnedBlock> blocks,NSNeuralShadowDataset::NSSampleBlockKind kind);

	static NSCaptureResult ValidateInputBlocksCore(const NSCaptureSamplePlan& sample,std::span<const NSCaptureOwnedBlock> blocks,bool allowGroundTruthBlocks);
	static NSCaptureResult ValidatePixelFeatures(const NSCaptureOwnedBlock& block, std::uint32_t workCount);
	static NSCaptureResult ValidateLightFeatures(const NSCaptureOwnedBlock& block, std::uint32_t workCount);
};

}
