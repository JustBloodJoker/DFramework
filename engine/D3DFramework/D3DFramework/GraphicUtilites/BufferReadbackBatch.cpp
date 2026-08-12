#include "../pch.h"

#include "BufferReadbackBatch.h"

namespace FD3DW {

    bool BuildBufferReadbackLayout(std::span<const std::uint64_t> byteSizes,std::vector<BufferReadbackLayoutEntry>& entries, std::uint64_t& totalBytes) {
    
        entries.clear();
        totalBytes = 0u;
    
        if (byteSizes.empty() || byteSizes.size() > BUFFER_READBACK_MAX_SLICES) return false;
    
        entries.reserve(byteSizes.size());
        for (const auto byteSize : byteSizes) {

            if (byteSize == 0u || totalBytes > std::numeric_limits<std::uint64_t>::max() - (BUFFER_READBACK_SLICE_ALIGNMENT - 1u)) return false;

            auto aligned = (totalBytes + BUFFER_READBACK_SLICE_ALIGNMENT - 1u) & ~(BUFFER_READBACK_SLICE_ALIGNMENT - 1u);

            if (aligned > std::numeric_limits<std::uint64_t>::max() - byteSize) return false;

            entries.push_back({ aligned, byteSize });
            totalBytes = aligned + byteSize;
        }
        return true;
    }

    bool BufferReadbackBatch::Create(ID3D12Device* device, std::span<const BufferReadbackSource> sources, std::unique_ptr<BufferReadbackBatch>& batch, std::string& error) {
        batch.reset();
        if (!device || sources.empty() || sources.size() > BUFFER_READBACK_MAX_SLICES) {
            error = "readback device and bounded source list are required";
            return false;
        }

        std::vector<std::uint64_t> byteSizes;
        byteSizes.reserve( sources.size() );
        for (const auto& source : sources) {

            if (!source.Resource || source.ByteSize == 0u || !source.Resource->GetResource()) {
                error = "readback source/resource size is invalid";
                return false;
            }

            const auto description = source.Resource->GetResourceDescription();
            if (description.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER ||
                source.SourceOffset > description.Width ||
                source.ByteSize > description.Width - source.SourceOffset) {

                error = "readback accepts only in-bounds buffer slices";
        
                return false;
            }
            byteSizes.push_back(source.ByteSize);
        }

        auto result = std::make_unique<BufferReadbackBatch>();
        if (!BuildBufferReadbackLayout(byteSizes, result->m_vLayout, result->m_uTotalBytes)) {
            error = "failed to build readback layout";
            return false;
        }

        result->m_vSources.reserve(sources.size());
        for (const auto& source : sources) {
            OwnedSource owned{};
            owned.Wrapper = source.Resource;
            owned.ResourceLifetime = source.Resource->GetResource();
            owned.SourceOffset = source.SourceOffset;
            owned.ByteSize = source.ByteSize;
            result->m_vSources.push_back(std::move(owned));
        }

        auto heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
        auto description = CD3DX12_RESOURCE_DESC::Buffer(result->m_uTotalBytes);
        hr = device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &description, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(result->m_pReadback.ReleaseAndGetAddressOf()));
        if (FAILED(hr)) {
            std::ostringstream stream;
            stream << "failed to create readback resource (bytes=" << result->m_uTotalBytes
                << ", hr=0x" << std::hex << std::uint32_t(hr) << ')';
            error = stream.str();
            return false;
        }
    
        batch = std::move(result);
        error.clear();

        return true;
    }

    bool BufferReadbackBatch::RecordCopies(ID3D12GraphicsCommandList* commandList, std::string& error) {
        if (!commandList || !m_pReadback || m_bRecorded) {
            error = "readback batch is missing, null, or already recorded";
            return false;
        }
    
        for (auto index = 0u; index < m_vSources.size(); ++index) {
            const auto& source = m_vSources[index];
            if (!source.Wrapper || !source.Wrapper->RecordBufferReadbackCopy(commandList, m_pReadback.Get(), m_vLayout[index].ReadbackOffset, source.SourceOffset, source.ByteSize)) {
                error = "readback source became invalid before recording";
                return false;
            }
        }

        m_bRecorded = true;
        error.clear();
        return true;
    }

    bool BufferReadbackBatch::ReadAfterCompletion(ID3D12Fence* completionFence, std::uint64_t completionValue, std::vector<std::vector<std::byte>>& slices, std::string& error) const {
        slices.clear();
        if (!m_bRecorded || !m_pReadback) {
            error = "readback copies have not been recorded";
            return false;
        }

        if (!completionFence || completionValue == 0u ||
            completionFence->GetCompletedValue() < completionValue) {
            error = "readback submission fence has not completed";
            return false;
        }

        D3D12_RANGE readRange{ 0u, SIZE_T(m_uTotalBytes) };
        void* mapped = nullptr;
        if (FAILED(m_pReadback->Map(0u, &readRange, &mapped)) || !mapped) {
            error = "failed to map completed readback resource";
            return false;
        }

        const auto* bytes = static_cast<const std::byte*>(mapped);
        slices.reserve(m_vLayout.size());
        for (const auto& entry : m_vLayout) {
            slices.emplace_back(bytes + entry.ReadbackOffset, bytes + entry.ReadbackOffset + entry.ByteSize);
        }

        D3D12_RANGE noWrites{ 0u, 0u };
        m_pReadback->Unmap(0u, &noWrites);
        error.clear();
        return true;
    }

    std::span<const BufferReadbackLayoutEntry> BufferReadbackBatch::GetLayout() const {
        return m_vLayout;
    }

    std::uint64_t BufferReadbackBatch::GetTotalBytes() const {
        return m_uTotalBytes;
    }

}
