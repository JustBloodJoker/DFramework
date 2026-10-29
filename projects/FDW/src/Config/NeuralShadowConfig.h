#pragma once

namespace FDWConfig::NeuralShadows {
inline constexpr wchar_t ModelPath[] = L"Content/ShadowRendering/NeuralModel/neural_shadow_model.onnx";
inline constexpr std::uint32_t WorkThreadCountX = 64u;
inline constexpr std::uint32_t WorkCountPass = 0u;
inline constexpr std::uint32_t WorkScanPass = 1u;
inline constexpr std::uint32_t WorkWritePass = 2u;
inline constexpr std::uint32_t BaselineThreadCountX = 64u;
inline constexpr std::uint32_t BaselineAtlasDescriptor = 0u;
inline constexpr std::uint32_t BaselineDepthDescriptor = 1u;
inline constexpr std::uint32_t BaselineNormalDescriptor = 2u;
} // namespace FDWConfig::NeuralShadows
