#include <D3DFramework/pch.h>

#include <D3DFramework/GraphicUtilites/BufferReadbackBatch.h>
#include <D3DFramework/GraphicUtilites/RenderThreadUtils/RenderThreadManager.h>
#include <D3DFramework/GraphicUtilites/StructuredBuffer.h>


bool Check(bool condition, const char* message) {
    if (condition) return true;
    
    std::cerr << "FAILED: " << message << '\n';
    return false;
}

int main() {
    wrl::ComPtr<IDXGIFactory4> factory;
    wrl::ComPtr<IDXGIAdapter> adapter;
    wrl::ComPtr<ID3D12Device> device;
    
    if (FAILED(CreateDXGIFactory2(0u, IID_PPV_ARGS(factory.ReleaseAndGetAddressOf()))) ||
        FAILED(factory->EnumWarpAdapter(IID_PPV_ARGS(adapter.ReleaseAndGetAddressOf()))) ||
        FAILED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(device.ReleaseAndGetAddressOf())))) {

        return 1;
    }

    auto source = FD3DW::StructuredBuffer::CreateStructuredBuffer<std::uint32_t>(device.Get(), 4u, false);
    const std::array values{ 0x11223344u, 0x55667788u, 0x99aabbccu, 0xddeeff00u };

    FD3DW::RenderThreadManager manager;
    manager.Init( device.Get() );

    auto uploadRecipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        [&source, &values, device](ID3D12GraphicsCommandList* list) {
            source->UploadData(device.Get(), list, values.data(), static_cast<UINT>(values.size()),
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        });

    auto upload = manager.Submit(uploadRecipe);


    std::string error;
    std::unique_ptr<FD3DW::BufferReadbackBatch> invalidBatch;
    const FD3DW::BufferReadbackSource invalid[] = { { source.get(), sizeof(values), sizeof(std::uint32_t) } };

    auto passed = Check(!FD3DW::BufferReadbackBatch::Create(device.Get(), invalid, invalidBatch, error), "out-of-range slice must be rejected");

    const FD3DW::BufferReadbackSource sources[] = {
        { source.get(), 0u, sizeof(std::uint32_t) },
        { source.get(), 2u * sizeof(std::uint32_t), 2u * sizeof(std::uint32_t) }
    };

    std::unique_ptr<FD3DW::BufferReadbackBatch> batch;
    passed &= Check(FD3DW::BufferReadbackBatch::Create(device.Get(), sources, batch, error), "valid multiple-slice batch must be created");
    
    if (!batch) return 1;

    auto recordSucceeded = std::make_shared<bool>(false);
    
    auto recordRecipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        [&batch, recordSucceeded](ID3D12GraphicsCommandList* list) {
            std::string localError;
            *recordSucceeded = batch->RecordCopies(list, localError);
        });

    auto readback = manager.Submit(recordRecipe, { upload }, true);


    std::vector<std::vector<std::byte>> slices;
    passed &= Check(!batch->ReadAfterCompletion(nullptr, 0u, slices, error), "read before a completed fence must fail without blocking");
    
    readback->WaitForExecute();
    
    passed &= Check(*recordSucceeded, "copy recording must succeed");
    
    passed &= Check(batch->ReadAfterCompletion( readback->GetFence(), readback->GetFenceValue(), slices, error), "completed readback must succeed");
    
    passed &= Check(source->GetTrackedState() == D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, "wrapper tracked state must be restored after readback");
    
    passed &= Check(slices.size() == 2u && slices[0].size() == 4u && slices[1].size() == 8u, "slice layout must be preserved");
    
    if (slices.size() == 2u) {
        std::uint32_t first = 0u;
        std::array<std::uint32_t, 2> tail{};
        
        std::memcpy(&first, slices[0].data(), sizeof(first));
        std::memcpy(tail.data(), slices[1].data(), sizeof(tail));

        passed &= Check(first == values[0] && tail[0] == values[2] && tail[1] == values[3], "readback values must match requested ranges");
    }

    manager.Shutdown();
    return passed ? 0 : 1;
}
