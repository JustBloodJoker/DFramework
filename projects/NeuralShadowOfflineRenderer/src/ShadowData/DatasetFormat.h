#pragma once

#include <Component/Light/LightComponentData.h>
#include <ShadowData/ShadowLightParameters.h>
#include <ShadowData/NeuralShadowData.h>

namespace FDW_SD::DatasetFormat {

inline constexpr std::uint32_t FORMAT_MAGIC = 0x3143'534eu;
inline constexpr std::uint32_t LAYOUT_VERSION = 1u;
inline constexpr std::uint32_t LAYOUT_METADATA_VERSION = 1u;
inline constexpr std::uint32_t LIGHT_LAYOUT_VERSION = 2u;
inline constexpr std::uint32_t SHADOW_METADATA_LAYOUT_VERSION = 1u;
inline constexpr std::uint32_t LITTLE_ENDIAN = 1u;
inline constexpr std::uint32_t VALIDITY_REASON_COUNT = 12u;

enum class NSDatasetLayoutErrorFlags : std::uint32_t {
	None = 0u,
	Identity = 1u << 0u,
	Versions = 1u << 1u,
	Strides = 1u << 2u,
	FeatureTensor = 1u << 3u,
	Output = 1u << 4u,
	Capacities = 1u << 5u,
	Validity = 1u << 6u,
	Serialization = 1u << 7u,
	DatasetMetadata = 1u << 8u,
	ModelMetadata = 1u << 9u
};

enum class NSDatasetLayoutErrorCode : std::uint32_t {
	None = 0u,
	Magic,
	ManifestVersion,
	LightLayoutVersion,
	ShadowMetadataLayoutVersion,
	RTReferenceVersion,
	WorkLayoutVersion,
	FeatureLayoutVersion,
	OutputLayoutVersion,
	LightStride,
	ShadowMetadataStride,
	RTQualityMetadataStride,
	WorkItemStride,
	WorkHeaderStride,
	FeaturePixelChannels,
	FeatureLightChannels,
	FeaturePatchWidth,
	FeaturePatchHeight,
	FeatureTensorLayout,
	FeatureStorageType,
	OutputHeaderStride,
	OutputVisibilityEncoding,
	OutputValidityEncoding,
	OutputValuesPerWorkItem,
	OutputLookupKind,
	OutputMaxLookupProbes,
	MaxWorkItems,
	MaxOutputValues,
	ValidityReasonCount,
	Endianness,
	PaddingField,
	DatasetMetadataVersion,
	DatasetMetadataByteSize,
	DatasetLayoutOffset,
	ModelMetadataVersion,
	ModelMetadataByteSize,
	ModelLayoutOffset
};

struct NSDatasetLayout {
	std::uint32_t Magic = 0u;
	std::uint32_t ManifestVersion = 0u;
	std::uint32_t LightLayoutVersion = 0u;
	std::uint32_t ShadowMetadataLayoutVersion = 0u;
	std::uint32_t RTReferenceVersion = 0u;
	std::uint32_t WorkLayoutVersion = 0u;
	std::uint32_t FeatureLayoutVersion = 0u;
	std::uint32_t OutputLayoutVersion = 0u;
	std::uint32_t LightStride = 0u;
	std::uint32_t ShadowMetadataStride = 0u;
	std::uint32_t RTQualityMetadataStride = 0u;
	std::uint32_t WorkItemStride = 0u;
	std::uint32_t WorkHeaderStride = 0u;
	std::uint32_t FeaturePixelChannels = 0u;
	std::uint32_t FeatureLightChannels = 0u;
	std::uint32_t FeaturePatchWidth = 0u;
	std::uint32_t FeaturePatchHeight = 0u;
	std::uint32_t FeatureTensorLayout = 0u;
	std::uint32_t FeatureStorageType = 0u;
	std::uint32_t OutputHeaderStride = 0u;
	std::uint32_t OutputVisibilityEncoding = 0u;
	std::uint32_t OutputValidityEncoding = 0u;
	std::uint32_t OutputValuesPerWorkItem = 0u;
	std::uint32_t OutputLookupKind = 0u;
	std::uint32_t OutputMaxLookupProbes = 0u;
	std::uint32_t MaxWorkItems = 0u;
	std::uint32_t MaxOutputValues = 0u;
	std::uint32_t ValidityReasonCount = 0u;
	std::uint32_t Endianness = 0u;
	std::uint32_t Padding0 = 0u;
	std::uint32_t Padding1 = 0u;
	std::uint32_t Padding2 = 0u;
};

struct NSDatasetLayoutMetadata {
	std::uint32_t MetadataVersion = LAYOUT_METADATA_VERSION;
	std::uint32_t MetadataByteSize = 0u;
	std::uint32_t LayoutOffset = 16u;
	std::uint32_t Padding0 = 0u;
	
	NSDatasetLayout Layout{};
};

struct NSModelLayoutMetadata {
	std::uint32_t MetadataVersion = LAYOUT_METADATA_VERSION;
	std::uint32_t MetadataByteSize = 0u;
	std::uint32_t LayoutOffset = 16u;
	std::uint32_t Padding0 = 0u;
	
	NSDatasetLayout Layout{};
};

struct NSDatasetLayoutValidation {
	std::uint32_t ErrorFlags = 0u;
	std::uint32_t FirstError = static_cast<std::uint32_t>(NSDatasetLayoutErrorCode::None);
	std::uint32_t Expected = 0u;
	std::uint32_t Actual = 0u;
};

class DatasetLayoutTools {
public:
	static NSDatasetLayout MakeLayout();
	static NSDatasetLayoutMetadata MakeDatasetMetadata();
	static NSModelLayoutMetadata MakeModelMetadata();
	
	static NSDatasetLayoutValidation ValidateLayout(const NSDatasetLayout& actual);
	static NSDatasetLayoutValidation ValidateDatasetMetadata(const NSDatasetLayoutMetadata& metadata);
	static NSDatasetLayoutValidation ValidateModelMetadata(const NSModelLayoutMetadata& metadata);
	
	static const char* GetFieldName(NSDatasetLayoutErrorCode code);
	
	static std::string DescribeValidation(const NSDatasetLayoutValidation& result);

protected:
	static void CheckField(std::uint32_t actual,std::uint32_t expected,NSDatasetLayoutErrorFlags flag,NSDatasetLayoutErrorCode code,NSDatasetLayoutValidation& result);
};

}
