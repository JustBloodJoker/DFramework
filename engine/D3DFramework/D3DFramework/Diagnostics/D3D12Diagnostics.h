#pragma once

#include "../d3dframework.h"

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace FD3DW {

    struct D3D12DebugMessage {
        D3D12_MESSAGE_CATEGORY Category = D3D12_MESSAGE_CATEGORY_APPLICATION_DEFINED;
        D3D12_MESSAGE_SEVERITY Severity = D3D12_MESSAGE_SEVERITY_INFO;
        D3D12_MESSAGE_ID Id = D3D12_MESSAGE_ID_UNKNOWN;
        std::uint64_t QueueIndex = 0u;
        std::string Description;
    };

    std::vector<D3D12DebugMessage> CollectD3D12Messages(ID3D12Device* device, std::uint64_t firstMessageIndex = 0u);

    std::string FormatD3D12Messages(std::span<const D3D12DebugMessage> messages, D3D12_MESSAGE_SEVERITY maximumSeverity = D3D12_MESSAGE_SEVERITY_MESSAGE);

    class D3D12MessageScope {
    public:
        explicit D3D12MessageScope(ID3D12Device* device = nullptr);

        std::vector<D3D12DebugMessage> Collect() const;
        std::uint64_t GetFirstMessageIndex() const;
        bool IsAvailable() const;

    private:
        Microsoft::WRL::ComPtr<ID3D12InfoQueue> m_pInfoQueue;
        std::uint64_t m_uFirstMessageIndex = 0u;
    };

}
