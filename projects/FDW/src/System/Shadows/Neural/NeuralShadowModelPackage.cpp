#include <pch.h>

#include <System/Shadows/Neural/NeuralShadowModelPackage.h>
#include <WinWindow/Utils/FileHash.h>

namespace NeuralShadowModel {

constexpr const char *GRAPH_SHA = "d2bc1a514d42db01520354a7652078454bf941f7654b3a3490226517f6d1bbfc";
constexpr const char *CHECKPOINT_SHA = "193c452432f257ffd7de41a41a11f3e112d00d05119f6306134911a7c5b76319";

bool PackageLoader::ReadText(const std::filesystem::path &path, std::string &text, std::string &error) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        error = "neural-shadow metadata is missing: " + path.string();
        return false;
    }

    std::ostringstream contents;
    contents << stream.rdbuf();
    text = contents.str();
    return true;
}

bool PackageLoader::ExtractString(const std::string &text, const char *key, std::string &value, std::string &error) {
    const std::regex expression("\\\"" + std::string(key) + "\\\"\\s*:\\s*\\\"([^\\\"]*)\\\"");
    std::smatch match;
    if (!std::regex_search(text, match, expression)) {
        error = "neural-shadow metadata key missing: " + std::string(key);
        return false;
    }

    value = match[1].str();
    return true;
}

bool PackageLoader::ExtractUnsigned(const std::string &text, const char *key, std::uint64_t &value, std::string &error) {
    const std::regex expression("\\\"" + std::string(key) + "\\\"\\s*:\\s*([0-9]+)");
    std::smatch match;
    if (!std::regex_search(text, match, expression)) {
        error = "neural-shadow numeric metadata key missing: " + std::string(key);
        return false;
    }

    const auto digits = match[1].str();
    const auto conversion = std::from_chars(digits.data(), digits.data() + digits.size(), value);
    if (conversion.ec != std::errc{} || conversion.ptr != digits.data() + digits.size()) {
        error = "neural-shadow numeric metadata value is invalid: " + std::string(key);
        return false;
    }

    return true;
}

bool PackageLoader::ReadBytes(const std::filesystem::path &path, std::vector<std::uint8_t> &bytes, std::string &error) {
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream) {
        error = "neural-shadow graph is missing: " + path.string();
        return false;
    }

    const auto end = stream.tellg();
    if (end <= 0) {
        error = "neural-shadow graph is empty";
        return false;
    }

    bytes.resize(static_cast<std::size_t>(end));
    stream.seekg(0);
    if (!stream.read(reinterpret_cast<char *>(bytes.data()), end)) {
        error = "neural-shadow graph could not be read: " + path.string();
        bytes.clear();
        return false;
    }

    return true;
}

bool PackageLoader::LoadAndValidate(const std::filesystem::path &graphPath, Package &package, std::string &error) {
    Package result{};
    result.GraphPath = graphPath;
    result.MetadataPath = graphPath;
    result.MetadataPath.replace_extension(L".json");

    std::string metadata;
    std::string format;
    std::string spatialAxes;
    std::string outputSemantics;
    std::uint64_t parameterCount = 0u;
    std::uint64_t receptiveField = 0u;
    std::uint64_t receptiveFieldRadius = 0u;
    
    if (!ReadText(result.MetadataPath, metadata, error) || !ExtractString(metadata, "architecture", result.Architecture, error) ||
        !ExtractString(metadata, "descriptor_layout", result.DescriptorLayout, error) ||
        !ExtractString(metadata, "hard_input_layout", result.HardInputLayout, error) ||
        !ExtractString(metadata, "runtime_model_id", result.RuntimeModelId, error) ||
        !ExtractString(metadata, "runtime_role", result.RuntimeRole, error) ||
        !ExtractString(metadata, "input_layout", result.InputLayout, error) ||
        !ExtractString(metadata, "output_layout", result.OutputLayout, error) ||
        !ExtractString(metadata, "graph_sha256", result.GraphSha256, error) ||
        !ExtractString(metadata, "checkpoint_sha256", result.CheckpointSha256, error) ||
        !ExtractString(metadata, "format", format, error) || !ExtractString(metadata, "spatial_axes", spatialAxes, error) ||
        !ExtractString(metadata, "output_semantics", outputSemantics, error) ||
        !ExtractUnsigned(metadata, "graph_bytes", result.GraphBytes, error) ||
        !ExtractUnsigned(metadata, "parameter_count", parameterCount, error) ||
        !ExtractUnsigned(metadata, "flops_64x64", result.Flops64x64, error) ||
        !ExtractUnsigned(metadata, "receptive_field", receptiveField, error) ||
        !ExtractUnsigned(metadata, "receptive_field_radius", receptiveFieldRadius, error)) {
    
        return false;
    
    }

    result.ParameterCount = static_cast<std::uint32_t>(parameterCount);
    result.ReceptiveField = static_cast<std::uint32_t>(receptiveField);
    result.ReceptiveFieldRadius = static_cast<std::uint32_t>(receptiveFieldRadius);
    
    if (format != "FDW_NEURAL_SHADOW_MODEL_PACKAGE" || result.Architecture != "SpatialVisibilityModel" || spatialAxes != "DYNAMIC_HW" ||
        outputSemantics != "PHYSICAL_VISIBILITY" || result.DescriptorLayout != "STRUCTURED_3X3" ||
        result.HardInputLayout != "BILINEAR_HALF_PIXEL" || result.RuntimeModelId != "SpatialVisibilityModel" ||
        result.RuntimeRole != "FROZEN_RUNTIME_MODEL" || result.InputLayout != "BASE9_DESCRIPTOR18_HARD1" ||
        result.OutputLayout != "PHYSICAL_VISIBILITY" || result.GraphSha256 != GRAPH_SHA || result.CheckpointSha256 != CHECKPOINT_SHA ||
        result.GraphBytes != 257815u || result.ParameterCount != 59661u || result.Flops64x64 != 484147200ull ||
        result.ReceptiveField != 33u || result.ReceptiveFieldRadius != 16u) {


        error = "frozen neural-shadow model identity mismatch";
        return false;
    }

    std::vector<std::uint8_t> bytes;
    if (!ReadBytes(graphPath, bytes, error)) {
        return false;
    }

    const auto actual = FDWWIN::FileHash::ToHex(FDWWIN::FileHash::Sha256(bytes.data(), bytes.size()));
    if (actual != result.GraphSha256 || bytes.size() != result.GraphBytes) {

        error = "neural-shadow graph SHA/size mismatch";
        return false;
    }

    package = std::move(result);
    error.clear();
    return true;
}

float PackageLoader::ResolvePhysicalVisibility(float modelOutput) noexcept {
    return std::clamp(modelOutput, 0.0f, 1.0f);
}

}
