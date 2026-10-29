#pragma once

namespace NSOfflineConfig::NeuralShadows {
inline constexpr std::uint32_t WorkThreadCountX = 64u;
inline constexpr std::uint32_t WorkCountPass = 0u;
inline constexpr std::uint32_t WorkScanPass = 1u;
inline constexpr std::uint32_t WorkWritePass = 2u;
inline constexpr std::uint32_t BaselineThreadCountX = 64u;
inline constexpr std::uint32_t BaselineAtlasDescriptor = 0u;
inline constexpr std::uint32_t BaselineDepthDescriptor = 1u;
inline constexpr std::uint32_t BaselineNormalDescriptor = 2u;
inline constexpr std::uint32_t InputCaptureWorkCapacity = 1024u;
inline constexpr std::uint32_t InputCaptureValuesPerWork = 256u;
inline constexpr std::uint32_t InputCaptureLayout = 2u;
inline constexpr std::uint32_t RTTargetDispatchLayout = 1u;
inline constexpr std::uint32_t RTTargetDispatchBatchCapacity = 65'535u;
inline constexpr std::uint32_t RTTargetDispatchBatchCount = 5u;
inline constexpr std::uint32_t RTTargetGeometryTextureDescriptors = 200u;
inline constexpr std::uint32_t RTTargetInitializePass = 0u;
inline constexpr std::uint32_t RTTargetTracePass = 1u;
inline constexpr std::uint32_t RTTargetFinalizePass = 2u;
inline constexpr std::uint32_t RTTargetParameterRecordCount = RTTargetDispatchBatchCount + 2u;
} // namespace NSOfflineConfig::NeuralShadows
