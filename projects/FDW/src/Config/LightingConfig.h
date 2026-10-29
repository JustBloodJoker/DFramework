#pragma once

namespace FDWConfig::Lighting {
inline constexpr std::uint32_t ClusterThreadsFirstPass = 1;
inline constexpr std::uint32_t ClusterThreadsSecondPass = 128;
inline constexpr std::uint32_t TileCountX = 16;
inline constexpr std::uint32_t TileCountY = 9;
inline constexpr std::uint32_t TileCountZ = 1; // Intentional full-depth 2D light tiles.
inline constexpr std::uint32_t MaxLightsPerTile = 300;
inline constexpr std::uint32_t BrdfLutSize = 256;
inline constexpr std::uint32_t BrdfLutSamples = 256;
inline constexpr std::uint32_t IrradianceCubemapSize = 32;
inline constexpr std::uint32_t PrefilteredCubemapSize = 128;
inline constexpr char LtcMatrixTexturePath[] = "Content/LTC/ltc_mat.dds";
inline constexpr char LtcAmplitudeTexturePath[] = "Content/LTC/ltc_amp.dds";
}
