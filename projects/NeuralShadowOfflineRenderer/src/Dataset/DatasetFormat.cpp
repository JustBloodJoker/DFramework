#include <pch.h>

#include <ShadowData/DatasetFormat.h>

namespace FDW_SD::DatasetFormat {

void DatasetLayoutTools::CheckField(std::uint32_t actual, std::uint32_t expected, NSDatasetLayoutErrorFlags flag,
                                    NSDatasetLayoutErrorCode code, NSDatasetLayoutValidation &result) {
    if (actual == expected) {
        return;
    }

    result.ErrorFlags |= static_cast<std::uint32_t>(flag);
    if (result.FirstError == static_cast<std::uint32_t>(NSDatasetLayoutErrorCode::None)) {
        result.FirstError = static_cast<std::uint32_t>(code);
        result.Expected = expected;
        result.Actual = actual;
    }
}

NSDatasetLayout DatasetLayoutTools::MakeLayout() {
    NSDatasetLayout layout{};
    layout.Magic = FORMAT_MAGIC;
    layout.ManifestVersion = LAYOUT_VERSION;
    layout.LightLayoutVersion = LIGHT_LAYOUT_VERSION;
    layout.ShadowMetadataLayoutVersion = SHADOW_METADATA_LAYOUT_VERSION;
    layout.RTReferenceVersion = NS_RT_REFERENCE_VERSION;
    layout.WorkLayoutVersion = FDW_SD::Work::DATA_LAYOUT_VERSION;
    layout.FeatureLayoutVersion = FDW_SD::Features::FEATURE_LAYOUT_VERSION;
    layout.OutputLayoutVersion = FDW_SD::Output::OUTPUT_LAYOUT_VERSION;
    layout.LightStride = sizeof(LightComponentData);
    layout.ShadowMetadataStride = sizeof(LightAtlasMeta);
    layout.RTQualityMetadataStride = sizeof(NSRTQualityMetadata);
    layout.WorkItemStride = sizeof(FDW_SD::Work::NSWorkItem);
    layout.WorkHeaderStride = sizeof(FDW_SD::Work::NSWorkDispatchHeader);
    layout.FeaturePixelChannels = FDW_SD::Features::PIXEL_CHANNEL_COUNT;
    layout.FeatureLightChannels = FDW_SD::Features::LIGHT_CHANNEL_COUNT;
    layout.FeaturePatchWidth = FDW_SD::Features::PATCH_WIDTH;
    layout.FeaturePatchHeight = FDW_SD::Features::PATCH_HEIGHT;
    layout.FeatureTensorLayout = static_cast<std::uint32_t>(FDW_SD::Features::NSFeatureTensorLayout::NCHW);
    layout.FeatureStorageType = static_cast<std::uint32_t>(FDW_SD::Features::NSFeatureStorageType::IEEEFloat16);
    layout.OutputHeaderStride = sizeof(FDW_SD::Output::NSOutputHeader);
    layout.OutputVisibilityEncoding = static_cast<std::uint32_t>(FDW_SD::Output::NSVisibilityEncoding::R16Float);
    layout.OutputValidityEncoding = static_cast<std::uint32_t>(FDW_SD::Output::NSValidityEncoding::R8Uint);
    layout.OutputValuesPerWorkItem = FDW_SD::Output::VALUES_PER_WORK_ITEM;
    layout.OutputLookupKind = static_cast<std::uint32_t>(FDW_SD::Output::NSOutputLookupKind::DirectPatchRangeBinarySearch);
    layout.OutputMaxLookupProbes = FDW_SD::Output::MAX_LOOKUP_PROBES;
    layout.MaxWorkItems = FDW_SD::Work::MAX_WORK_ITEMS;
    layout.MaxOutputValues = FDW_SD::Output::MAX_OUTPUT_VALUES;
    layout.ValidityReasonCount = VALIDITY_REASON_COUNT;
    layout.Endianness = LITTLE_ENDIAN;
    
    return layout;
}

NSDatasetLayoutMetadata DatasetLayoutTools::MakeDatasetMetadata() {
    NSDatasetLayoutMetadata metadata{};
    metadata.MetadataByteSize = sizeof(NSDatasetLayoutMetadata);
    metadata.LayoutOffset = offsetof(NSDatasetLayoutMetadata, Layout);
    metadata.Layout = MakeLayout();
   
    return metadata;
}

NSModelLayoutMetadata DatasetLayoutTools::MakeModelMetadata() {
    NSModelLayoutMetadata metadata{};
    metadata.MetadataByteSize = sizeof(NSModelLayoutMetadata);
    metadata.LayoutOffset = offsetof(NSModelLayoutMetadata, Layout);
    metadata.Layout = MakeLayout();
    
    return metadata;
}

NSDatasetLayoutValidation DatasetLayoutTools::ValidateLayout(const NSDatasetLayout &actual) {
    const auto expected = MakeLayout();
    NSDatasetLayoutValidation result{};

#define NS_CHECK_LAYOUT_FIELD(field, flag, code) CheckField(actual.field, expected.field, flag, NSDatasetLayoutErrorCode::code, result)
    
    NS_CHECK_LAYOUT_FIELD(Magic, NSDatasetLayoutErrorFlags::Identity, Magic);
    NS_CHECK_LAYOUT_FIELD(ManifestVersion, NSDatasetLayoutErrorFlags::Versions, ManifestVersion);
    NS_CHECK_LAYOUT_FIELD(LightLayoutVersion, NSDatasetLayoutErrorFlags::Versions, LightLayoutVersion);
    NS_CHECK_LAYOUT_FIELD(ShadowMetadataLayoutVersion, NSDatasetLayoutErrorFlags::Versions, ShadowMetadataLayoutVersion);
    NS_CHECK_LAYOUT_FIELD(RTReferenceVersion, NSDatasetLayoutErrorFlags::Versions, RTReferenceVersion);
    NS_CHECK_LAYOUT_FIELD(WorkLayoutVersion, NSDatasetLayoutErrorFlags::Versions, WorkLayoutVersion);
    NS_CHECK_LAYOUT_FIELD(FeatureLayoutVersion, NSDatasetLayoutErrorFlags::Versions, FeatureLayoutVersion);
    NS_CHECK_LAYOUT_FIELD(OutputLayoutVersion, NSDatasetLayoutErrorFlags::Versions, OutputLayoutVersion);
    NS_CHECK_LAYOUT_FIELD(LightStride, NSDatasetLayoutErrorFlags::Strides, LightStride);
    NS_CHECK_LAYOUT_FIELD(ShadowMetadataStride, NSDatasetLayoutErrorFlags::Strides, ShadowMetadataStride);
    NS_CHECK_LAYOUT_FIELD(RTQualityMetadataStride, NSDatasetLayoutErrorFlags::Strides, RTQualityMetadataStride);
    NS_CHECK_LAYOUT_FIELD(WorkItemStride, NSDatasetLayoutErrorFlags::Strides, WorkItemStride);
    NS_CHECK_LAYOUT_FIELD(WorkHeaderStride, NSDatasetLayoutErrorFlags::Strides, WorkHeaderStride);
    NS_CHECK_LAYOUT_FIELD(FeaturePixelChannels, NSDatasetLayoutErrorFlags::FeatureTensor, FeaturePixelChannels);
    NS_CHECK_LAYOUT_FIELD(FeatureLightChannels, NSDatasetLayoutErrorFlags::FeatureTensor, FeatureLightChannels);
    NS_CHECK_LAYOUT_FIELD(FeaturePatchWidth, NSDatasetLayoutErrorFlags::FeatureTensor, FeaturePatchWidth);
    NS_CHECK_LAYOUT_FIELD(FeaturePatchHeight, NSDatasetLayoutErrorFlags::FeatureTensor, FeaturePatchHeight);
    NS_CHECK_LAYOUT_FIELD(FeatureTensorLayout, NSDatasetLayoutErrorFlags::FeatureTensor, FeatureTensorLayout);
    NS_CHECK_LAYOUT_FIELD(FeatureStorageType, NSDatasetLayoutErrorFlags::FeatureTensor, FeatureStorageType);
    NS_CHECK_LAYOUT_FIELD(OutputHeaderStride, NSDatasetLayoutErrorFlags::Output, OutputHeaderStride);
    NS_CHECK_LAYOUT_FIELD(OutputVisibilityEncoding, NSDatasetLayoutErrorFlags::Output, OutputVisibilityEncoding);
    NS_CHECK_LAYOUT_FIELD(OutputValidityEncoding, NSDatasetLayoutErrorFlags::Output, OutputValidityEncoding);
    NS_CHECK_LAYOUT_FIELD(OutputValuesPerWorkItem, NSDatasetLayoutErrorFlags::Output, OutputValuesPerWorkItem);
    NS_CHECK_LAYOUT_FIELD(OutputLookupKind, NSDatasetLayoutErrorFlags::Output, OutputLookupKind);
    NS_CHECK_LAYOUT_FIELD(OutputMaxLookupProbes, NSDatasetLayoutErrorFlags::Output, OutputMaxLookupProbes);
    NS_CHECK_LAYOUT_FIELD(MaxWorkItems, NSDatasetLayoutErrorFlags::Capacities, MaxWorkItems);
    NS_CHECK_LAYOUT_FIELD(MaxOutputValues, NSDatasetLayoutErrorFlags::Capacities, MaxOutputValues);
    NS_CHECK_LAYOUT_FIELD(ValidityReasonCount, NSDatasetLayoutErrorFlags::Validity, ValidityReasonCount);
    NS_CHECK_LAYOUT_FIELD(Endianness, NSDatasetLayoutErrorFlags::Serialization, Endianness);
    NS_CHECK_LAYOUT_FIELD(Padding0, NSDatasetLayoutErrorFlags::Serialization, PaddingField);
    NS_CHECK_LAYOUT_FIELD(Padding1, NSDatasetLayoutErrorFlags::Serialization, PaddingField);
    NS_CHECK_LAYOUT_FIELD(Padding2, NSDatasetLayoutErrorFlags::Serialization, PaddingField);

#undef NS_CHECK_LAYOUT_FIELD

    return result;
}

NSDatasetLayoutValidation DatasetLayoutTools::ValidateDatasetMetadata(const NSDatasetLayoutMetadata &metadata) {
    NSDatasetLayoutValidation result{};
    CheckField(metadata.MetadataVersion, LAYOUT_METADATA_VERSION, NSDatasetLayoutErrorFlags::DatasetMetadata,
               NSDatasetLayoutErrorCode::DatasetMetadataVersion, result);
    
    CheckField(metadata.MetadataByteSize, sizeof(NSDatasetLayoutMetadata), NSDatasetLayoutErrorFlags::DatasetMetadata,
               NSDatasetLayoutErrorCode::DatasetMetadataByteSize, result);
    
    CheckField(metadata.LayoutOffset, offsetof(NSDatasetLayoutMetadata, Layout), NSDatasetLayoutErrorFlags::DatasetMetadata,
               NSDatasetLayoutErrorCode::DatasetLayoutOffset, result);
    
    CheckField(metadata.Padding0, 0u, NSDatasetLayoutErrorFlags::Serialization, NSDatasetLayoutErrorCode::PaddingField, result);
    
    if (result.ErrorFlags != 0u) {
        return result;
    }

    return ValidateLayout(metadata.Layout);
}

NSDatasetLayoutValidation DatasetLayoutTools::ValidateModelMetadata(const NSModelLayoutMetadata &metadata) {
    NSDatasetLayoutValidation result{};
    
    CheckField(metadata.MetadataVersion, LAYOUT_METADATA_VERSION, NSDatasetLayoutErrorFlags::ModelMetadata,
               NSDatasetLayoutErrorCode::ModelMetadataVersion, result);
    
    CheckField(metadata.MetadataByteSize, sizeof(NSModelLayoutMetadata), NSDatasetLayoutErrorFlags::ModelMetadata,
               NSDatasetLayoutErrorCode::ModelMetadataByteSize, result);
   
    CheckField(metadata.LayoutOffset, offsetof(NSModelLayoutMetadata, Layout), NSDatasetLayoutErrorFlags::ModelMetadata,
               NSDatasetLayoutErrorCode::ModelLayoutOffset, result);
    
    CheckField(metadata.Padding0, 0u, NSDatasetLayoutErrorFlags::Serialization, NSDatasetLayoutErrorCode::PaddingField, result);
    
    if (result.ErrorFlags != 0u) {
        return result;
    }

    return ValidateLayout(metadata.Layout);
}

const char *DatasetLayoutTools::GetFieldName(NSDatasetLayoutErrorCode code) {
    switch (code) {
    case NSDatasetLayoutErrorCode::None:
        return "none";
    case NSDatasetLayoutErrorCode::Magic:
        return "magic";
    case NSDatasetLayoutErrorCode::ManifestVersion:
        return "manifestVersion";
    case NSDatasetLayoutErrorCode::LightLayoutVersion:
        return "lightLayoutVersion";
    case NSDatasetLayoutErrorCode::ShadowMetadataLayoutVersion:
        return "shadowMetadataLayoutVersion";
    case NSDatasetLayoutErrorCode::RTReferenceVersion:
        return "rtReferenceVersion";
    case NSDatasetLayoutErrorCode::WorkLayoutVersion:
        return "workLayoutVersion";
    case NSDatasetLayoutErrorCode::FeatureLayoutVersion:
        return "featureLayoutVersion";
    case NSDatasetLayoutErrorCode::OutputLayoutVersion:
        return "outputLayoutVersion";
    case NSDatasetLayoutErrorCode::LightStride:
        return "lightStride";
    case NSDatasetLayoutErrorCode::ShadowMetadataStride:
        return "shadowMetadataStride";
    case NSDatasetLayoutErrorCode::RTQualityMetadataStride:
        return "rtQualityMetadataStride";
    case NSDatasetLayoutErrorCode::WorkItemStride:
        return "workItemStride";
    case NSDatasetLayoutErrorCode::WorkHeaderStride:
        return "workHeaderStride";
    case NSDatasetLayoutErrorCode::FeaturePixelChannels:
        return "featurePixelChannels";
    case NSDatasetLayoutErrorCode::FeatureLightChannels:
        return "featureLightChannels";
    case NSDatasetLayoutErrorCode::FeaturePatchWidth:
        return "featurePatchWidth";
    case NSDatasetLayoutErrorCode::FeaturePatchHeight:
        return "featurePatchHeight";
    case NSDatasetLayoutErrorCode::FeatureTensorLayout:
        return "featureTensorLayout";
    case NSDatasetLayoutErrorCode::FeatureStorageType:
        return "featureStorageType";
    case NSDatasetLayoutErrorCode::OutputHeaderStride:
        return "outputHeaderStride";
    case NSDatasetLayoutErrorCode::OutputVisibilityEncoding:
        return "outputVisibilityEncoding";
    case NSDatasetLayoutErrorCode::OutputValidityEncoding:
        return "outputValidityEncoding";
    case NSDatasetLayoutErrorCode::OutputValuesPerWorkItem:
        return "outputValuesPerWorkItem";
    case NSDatasetLayoutErrorCode::OutputLookupKind:
        return "outputLookupKind";
    case NSDatasetLayoutErrorCode::OutputMaxLookupProbes:
        return "outputMaxLookupProbes";
    case NSDatasetLayoutErrorCode::MaxWorkItems:
        return "maxWorkItems";
    case NSDatasetLayoutErrorCode::MaxOutputValues:
        return "maxOutputValues";
    case NSDatasetLayoutErrorCode::ValidityReasonCount:
        return "validityReasonCount";
    case NSDatasetLayoutErrorCode::Endianness:
        return "endianness";
    case NSDatasetLayoutErrorCode::PaddingField:
        return "padding";
    case NSDatasetLayoutErrorCode::DatasetMetadataVersion:
        return "datasetLayoutMetadataVersion";
    case NSDatasetLayoutErrorCode::DatasetMetadataByteSize:
        return "datasetLayoutMetadataByteSize";
    case NSDatasetLayoutErrorCode::DatasetLayoutOffset:
        return "datasetLayoutOffset";
    case NSDatasetLayoutErrorCode::ModelMetadataVersion:
        return "modelLayoutMetadataVersion";
    case NSDatasetLayoutErrorCode::ModelMetadataByteSize:
        return "modelLayoutMetadataByteSize";
    case NSDatasetLayoutErrorCode::ModelLayoutOffset:
        return "modelLayoutOffset";
    }
    return "unknown";
}

std::string DatasetLayoutTools::DescribeValidation(const NSDatasetLayoutValidation &result) {
    if (result.ErrorFlags == 0u) {
        return "valid";
    }

    const auto code = static_cast<NSDatasetLayoutErrorCode>(result.FirstError);
    return std::string(GetFieldName(code)) + " mismatch: expected " + std::to_string(result.Expected) + ", actual " + std::to_string(result.Actual);
}

} // namespace FDW_SD::DatasetFormat
