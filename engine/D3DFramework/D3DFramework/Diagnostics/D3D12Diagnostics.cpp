#include "../pch.h"

#include "D3D12Diagnostics.h"


namespace FD3DW {

    std::vector<D3D12DebugMessage> CollectFromQueue(ID3D12InfoQueue* infoQueue, std::uint64_t firstMessageIndex) {
        std::vector<D3D12DebugMessage> messages;

        if (!infoQueue) return messages;
        
        const auto count = infoQueue->GetNumStoredMessagesAllowedByRetrievalFilter();
        
        firstMessageIndex = std::min(firstMessageIndex, count);
        messages.reserve( std::size_t(count - firstMessageIndex) );
        
        for (auto index = firstMessageIndex; index < count; ++index) {
            SIZE_T byteCount = 0u;

            if (FAILED(infoQueue->GetMessage(index, nullptr, &byteCount)) || byteCount == 0u) continue;
            
            std::vector<std::byte> storage(byteCount);
            auto source = reinterpret_cast<D3D12_MESSAGE*>(storage.data());
            
            if (FAILED(infoQueue->GetMessage(index, source, &byteCount))) continue;
            
            D3D12DebugMessage message{};
            message.Category = source->Category;
            message.Severity = source->Severity;
            message.Id = source->ID;
            message.QueueIndex = index;
            if (source->pDescription) message.Description = source->pDescription;
            
            messages.push_back(std::move(message));
        }
        return messages;
    }


std::vector<D3D12DebugMessage> CollectD3D12Messages(ID3D12Device* device, std::uint64_t firstMessageIndex) {
    Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue;
    
    if (!device || FAILED(device->QueryInterface(IID_PPV_ARGS(infoQueue.ReleaseAndGetAddressOf())))) return {};
    
    return CollectFromQueue(infoQueue.Get(), firstMessageIndex);
}

std::string FormatD3D12Messages(std::span<const D3D12DebugMessage> messages, D3D12_MESSAGE_SEVERITY maximumSeverity) {
    std::ostringstream stream;
    auto first = true;

    for (const auto& message : messages) {
        if (message.Severity > maximumSeverity) continue;
    
        if (!first) stream << " | ";
        
        first = false;
        
        stream << "D3D12[severity=" << unsigned(message.Severity)
            << ",category=" << unsigned(message.Category)
            << ",id=" << unsigned(message.Id)
            << ",index=" << message.QueueIndex << "]: " << message.Description;
    }
    
    return stream.str();
}

D3D12MessageScope::D3D12MessageScope(ID3D12Device* device) {
    if (!device || FAILED(device->QueryInterface(IID_PPV_ARGS(m_pInfoQueue.ReleaseAndGetAddressOf())))) {
        return;
    }
    m_uFirstMessageIndex = m_pInfoQueue->GetNumStoredMessagesAllowedByRetrievalFilter();
}

std::vector<D3D12DebugMessage> D3D12MessageScope::Collect() const {
    return CollectFromQueue(m_pInfoQueue.Get(), m_uFirstMessageIndex);
}

std::uint64_t D3D12MessageScope::GetFirstMessageIndex() const {
    return m_uFirstMessageIndex;
}

bool D3D12MessageScope::IsAvailable() const {
    return m_pInfoQueue != nullptr;
}

}
