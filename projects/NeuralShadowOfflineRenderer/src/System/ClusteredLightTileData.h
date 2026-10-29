#pragma once

#include <algorithm>

namespace NSClusteredLightTiles {

inline constexpr std::uint32_t DATA_LAYOUT_VERSION = 1u;
inline constexpr std::uint32_t GRID_WIDTH = 16u;
inline constexpr std::uint32_t GRID_HEIGHT = 9u;
inline constexpr std::uint32_t GRID_DEPTH = 1u;
inline constexpr std::uint32_t TILE_COUNT = GRID_WIDTH * GRID_HEIGHT;
inline constexpr std::uint32_t MAX_STORED_LIGHTS_PER_TILE = 300u;

constexpr std::uint32_t ComputeTileCoordinate(std::uint32_t pixel,std::uint32_t extent,std::uint32_t tileCount) {
	
	if (extent == 0u || tileCount == 0u) return 0u;
	
	const auto clampedPixel = std::min(pixel, extent - 1u);
	const auto scaled = static_cast<std::uint64_t>(clampedPixel) * tileCount;
	return std::min(static_cast<std::uint32_t>(scaled / extent), tileCount - 1u);
}

constexpr std::uint32_t ComputeTileIndex(std::uint32_t pixelX,std::uint32_t pixelY,std::uint32_t screenWidth,std::uint32_t screenHeight) {
	
	const auto tileX = ComputeTileCoordinate(pixelX, screenWidth, GRID_WIDTH);
	const auto tileY = ComputeTileCoordinate(pixelY, screenHeight, GRID_HEIGHT);
	return tileX + tileY * GRID_WIDTH;
}

constexpr float ScreenToNdcY(float screenY, float screenHeight) {
	return screenHeight > 0.0f ? 1.0f - (2.0f * screenY / screenHeight) : 0.0f;
}

struct TileCapacityResult {
	std::uint32_t Affecting = 0u;
	std::uint32_t Stored = 0u;
	std::uint32_t Dropped = 0u;
	std::uint32_t Overflowed = 0u;
};

constexpr TileCapacityResult ClassifyTileCapacity(std::uint32_t affecting) {
	const auto stored = std::min(affecting, MAX_STORED_LIGHTS_PER_TILE);
	return {
		affecting,
		stored,
		affecting - stored,
		affecting > MAX_STORED_LIGHTS_PER_TILE ? 1u : 0u
	};
}

}
