#pragma once

namespace FDWConfig::RendererLimits {
inline constexpr wchar_t WindowTitle[] = L"FDW";
inline constexpr int WindowStartWidth = 1600;
inline constexpr int WindowStartHeight = 900;
inline constexpr int MinimumSceneWidth = 320;
inline constexpr int MinimumUIWidth = 280;
inline constexpr int FixedUIWidth = 430;
inline constexpr int SelectionDragThresholdPixels = 4;
inline constexpr std::uint32_t RenderWorkerCount = 8;
inline constexpr std::uint32_t InFlightFrameCount = 1;
inline constexpr std::uint32_t TextureHeapNodeMask = 0;
inline constexpr std::uint32_t TextureHeapPrecacheSize = 200;
inline constexpr std::uint32_t BaseRenderableBlasHitGroupIndex = 0;
inline constexpr std::uint32_t GpuSkinningDispatchSize = 128;
inline constexpr std::uint32_t HizCopyDepthThreadX = 8;
inline constexpr std::uint32_t HizCopyDepthThreadY = 8;
inline constexpr std::uint32_t GpuCullingThreadGroupSize = 64;
inline constexpr std::uint32_t TaaJitterPhaseCount = 16;
inline constexpr float TaaPreferredFrameRate = 80.0f;
inline constexpr wchar_t DefaultScenePath[] = L"Content/";
}
