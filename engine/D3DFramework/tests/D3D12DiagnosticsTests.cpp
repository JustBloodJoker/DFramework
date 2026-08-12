#include <D3DFramework/Diagnostics/D3D12Diagnostics.h>

#include <iostream>

int main() {

    if ( !FD3DW::CollectD3D12Messages(nullptr).empty() ) return 1;
    
    const FD3DW::D3D12MessageScope unavailableScope;
    if ( unavailableScope.IsAvailable() || !unavailableScope.Collect().empty() ) return 2;

    const FD3DW::D3D12DebugMessage messages[] = {

        { D3D12_MESSAGE_CATEGORY_EXECUTION, D3D12_MESSAGE_SEVERITY_ERROR,
          D3D12_MESSAGE_ID_COMMAND_LIST_DRAW_ROOT_SIGNATURE_NOT_SET, 17u, "root signature" },
        { D3D12_MESSAGE_CATEGORY_STATE_CREATION, D3D12_MESSAGE_SEVERITY_INFO,
          D3D12_MESSAGE_ID_UNKNOWN, 18u, "informational" }
    
    };
    
    const auto errors = FD3DW::FormatD3D12Messages(messages, D3D12_MESSAGE_SEVERITY_ERROR);
    if (errors.find("severity=") == std::string::npos ||
        errors.find("category=") == std::string::npos ||
        errors.find("id=") == std::string::npos ||
        errors.find("root signature") == std::string::npos ||
        errors.find("informational") != std::string::npos) return 3;
    
    return 0;
}
