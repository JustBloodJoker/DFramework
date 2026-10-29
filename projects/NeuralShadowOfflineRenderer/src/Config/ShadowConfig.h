#pragma once

#include <dxgiformat.h>

namespace NSOfflineConfig::Shadows {
inline constexpr DXGI_FORMAT AtlasFormat = DXGI_FORMAT_R16_FLOAT;
inline constexpr DXGI_FORMAT AtlasLightIndexFormat = DXGI_FORMAT_R8_UINT;
using AtlasLightIndex = std::uint8_t;
inline constexpr std::uint32_t AtlasWidth = 8192;
inline constexpr std::uint32_t AtlasHeight = 8192;
inline constexpr std::uint32_t MaximumTileWidth = 256;
inline constexpr std::uint32_t MinimumTileWidth = 64;
}
