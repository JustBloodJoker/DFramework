#pragma once

namespace NeuralShadowModel {

struct Package {
    std::filesystem::path GraphPath;
    std::filesystem::path MetadataPath;
    std::string RuntimeModelId;
    std::string RuntimeRole;
    std::string Architecture;
    std::string InputLayout;
    std::string OutputLayout;
    std::string DescriptorLayout;
    std::string HardInputLayout;
    std::string GraphSha256;
    std::string CheckpointSha256;
    std::uint64_t GraphBytes = 0u;
    std::uint32_t ParameterCount = 0u;
    std::uint64_t Flops64x64 = 0u;
    std::uint32_t ReceptiveField = 0u;
    std::uint32_t ReceptiveFieldRadius = 0u;
};

class PackageLoader {
  public:
    static bool LoadAndValidate(const std::filesystem::path &graphPath, Package &package, std::string &error);
    static float ResolvePhysicalVisibility(float modelOutput) noexcept;

  protected:
    static bool ReadText(const std::filesystem::path &path, std::string &text, std::string &error);
    static bool ExtractString(const std::string &text, const char *key, std::string &value, std::string &error);
    static bool ExtractUnsigned(const std::string &text, const char *key, std::uint64_t &value, std::string &error);
    static bool ReadBytes(const std::filesystem::path &path, std::vector<std::uint8_t> &bytes, std::string &error);
};

} // namespace NeuralShadowModel
