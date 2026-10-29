#pragma once

namespace NSOfflineConfig::RendererLimits {
inline constexpr wchar_t WindowTitle[] = L"Neural Shadow Offline Renderer";
inline constexpr std::uint32_t RenderWorkerCount = 8;
inline constexpr std::uint32_t TextureHeapNodeMask = 0;
inline constexpr std::uint32_t TextureHeapPrecacheSize = 200;
inline constexpr std::uint32_t BaseRenderableBlasHitGroupIndex = 0;
}
