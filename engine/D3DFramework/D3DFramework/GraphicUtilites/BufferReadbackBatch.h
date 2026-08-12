#pragma once

#include "FResource.h"

namespace FD3DW {

#define BUFFER_READBACK_MAX_SLICES 32u
#define BUFFER_READBACK_SLICE_ALIGNMENT 256u

    struct BufferReadbackLayoutEntry {
        std::uint64_t ReadbackOffset = 0u;
        std::uint64_t ByteSize = 0u;
    };

    struct BufferReadbackSource {
        FResource* Resource = nullptr;
        std::uint64_t SourceOffset = 0u;
        std::uint64_t ByteSize = 0u;
    };

    bool BuildBufferReadbackLayout(std::span<const std::uint64_t> byteSizes,std::vector<BufferReadbackLayoutEntry>& entries,std::uint64_t& totalBytes);

    class BufferReadbackBatch {
    public:
        static bool Create(ID3D12Device* device, std::span<const BufferReadbackSource> sources, std::unique_ptr<BufferReadbackBatch>& batch, std::string& error);

        bool RecordCopies(ID3D12GraphicsCommandList* commandList, std::string& error);
        bool ReadAfterCompletion(ID3D12Fence* completionFence,std::uint64_t completionValue,std::vector<std::vector<std::byte>>& slices,std::string& error) const;

        std::span<const BufferReadbackLayoutEntry> GetLayout() const;
        std::uint64_t GetTotalBytes() const;

    protected:
        struct OwnedSource {
            FResource* Wrapper = nullptr;
            wrl::ComPtr<ID3D12Resource> ResourceLifetime;
            std::uint64_t SourceOffset = 0u;
            std::uint64_t ByteSize = 0u;
        };

        std::vector<OwnedSource> m_vSources;

    protected:
        std::vector<BufferReadbackLayoutEntry> m_vLayout;
        wrl::ComPtr<ID3D12Resource> m_pReadback;
        std::uint64_t m_uTotalBytes = 0u;
        bool m_bRecorded = false;
    };

}
