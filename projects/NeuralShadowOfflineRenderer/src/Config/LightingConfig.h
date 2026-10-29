#pragma once

namespace NSOfflineConfig::Lighting {
inline constexpr std::uint32_t ClusterThreadsSecondPass = 128;
inline constexpr std::uint32_t TileCountX = 16;
inline constexpr std::uint32_t TileCountY = 9;
inline constexpr std::uint32_t TileCountZ = 1; // Intentional full-depth 2D light tiles.
inline constexpr std::uint32_t MaxLightsPerTile = 300;
}
