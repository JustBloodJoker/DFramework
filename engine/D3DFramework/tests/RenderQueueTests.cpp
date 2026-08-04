#include <D3DFramework/GraphicUtilites/RenderThreadUtils/RenderThreadManager.h>

#include <cstdint>
#include <cstring>
#include <iostream>
#include <string_view>

namespace {

using Microsoft::WRL::ComPtr;

bool Check(bool condition, std::string_view message)
{
    if (condition) return true;
    std::cerr << "FAILED: " << message << '\n';
    return false;
}

bool CheckHR(HRESULT result, std::string_view operation)
{
    if (SUCCEEDED(result)) return true;
    std::cerr << "FAILED: " << operation << " (HRESULT=0x" << std::hex
              << static_cast<unsigned long>(result) << ")\n";
    return false;
}

D3D12_HEAP_PROPERTIES HeapProperties(D3D12_HEAP_TYPE type)
{
    D3D12_HEAP_PROPERTIES properties{};
    properties.Type = type;
    properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    properties.CreationNodeMask = 1;
    properties.VisibleNodeMask = 1;
    return properties;
}

D3D12_RESOURCE_DESC BufferDescription(UINT64 size)
{
    D3D12_RESOURCE_DESC description{};
    description.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    description.Width = size;
    description.Height = 1;
    description.DepthOrArraySize = 1;
    description.MipLevels = 1;
    description.Format = DXGI_FORMAT_UNKNOWN;
    description.SampleDesc = { 1, 0 };
    description.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    return description;
}

bool CreateBuffer(
    ID3D12Device* device,
    D3D12_HEAP_TYPE heapType,
    D3D12_RESOURCE_STATES initialState,
    ComPtr<ID3D12Resource>& resource)
{
    const auto heapProperties = HeapProperties(heapType);
    const auto description = BufferDescription(sizeof(std::uint32_t));
    return CheckHR(
        device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &description,
            initialState,
            nullptr,
            IID_PPV_ARGS(resource.ReleaseAndGetAddressOf())),
        "CreateCommittedResource");
}

D3D12_RESOURCE_BARRIER Transition(
    ID3D12Resource* resource,
    D3D12_RESOURCE_STATES before,
    D3D12_RESOURCE_STATES after)
{
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = resource;
    barrier.Transition.StateBefore = before;
    barrier.Transition.StateAfter = after;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    return barrier;
}

} // namespace

int main()
{
    ComPtr<IDXGIFactory4> factory;
    if (!CheckHR(CreateDXGIFactory2(0, IID_PPV_ARGS(factory.ReleaseAndGetAddressOf())), "CreateDXGIFactory2")) return 1;

    ComPtr<IDXGIAdapter> warpAdapter;
    if (!CheckHR(factory->EnumWarpAdapter(IID_PPV_ARGS(warpAdapter.ReleaseAndGetAddressOf())), "EnumWarpAdapter")) return 1;

    ComPtr<ID3D12Device> device;
    if (!CheckHR(
            D3D12CreateDevice(warpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(device.ReleaseAndGetAddressOf())),
            "D3D12CreateDevice")) {
        return 1;
    }

    ComPtr<ID3D12Resource> upload;
    ComPtr<ID3D12Resource> directOutput;
    ComPtr<ID3D12Resource> computeOutput;
    ComPtr<ID3D12Resource> readback;
    if (!CreateBuffer(device.Get(), D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, upload)
        || !CreateBuffer(device.Get(), D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON, directOutput)
        || !CreateBuffer(device.Get(), D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON, computeOutput)
        || !CreateBuffer(device.Get(), D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_STATE_COPY_DEST, readback)) {
        return 1;
    }

    constexpr std::uint32_t expectedValue = 0x51A7C0DEu;
    void* uploadData = nullptr;
    const D3D12_RANGE noReadRange{ 0, 0 };
    if (!CheckHR(upload->Map(0, &noReadRange, &uploadData), "Map upload buffer")) return 1;
    std::memcpy(uploadData, &expectedValue, sizeof(expectedValue));
    upload->Unmap(0, nullptr);

    FD3DW::RenderThreadManager manager;
    manager.Init(device.Get());

    bool passed = true;

    auto directWriteRecipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        [upload, directOutput](ID3D12GraphicsCommandList* list) {
            auto toCopyDest = Transition(
                directOutput.Get(),
                D3D12_RESOURCE_STATE_COMMON,
                D3D12_RESOURCE_STATE_COPY_DEST);
            list->ResourceBarrier(1, &toCopyDest);
            list->CopyBufferRegion(directOutput.Get(), 0, upload.Get(), 0, sizeof(std::uint32_t));
            auto toCopySource = Transition(
                directOutput.Get(),
                D3D12_RESOURCE_STATE_COPY_DEST,
                D3D12_RESOURCE_STATE_COPY_SOURCE);
            list->ResourceBarrier(1, &toCopySource);
        });
    auto directWrite = manager.Submit(directWriteRecipe);

    auto computeCopyRecipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(
        D3D12_COMMAND_LIST_TYPE_COMPUTE,
        [directOutput, computeOutput](ID3D12GraphicsCommandList* list) {
            auto toCopyDest = Transition(
                computeOutput.Get(),
                D3D12_RESOURCE_STATE_COMMON,
                D3D12_RESOURCE_STATE_COPY_DEST);
            list->ResourceBarrier(1, &toCopyDest);
            list->CopyBufferRegion(computeOutput.Get(), 0, directOutput.Get(), 0, sizeof(std::uint32_t));
            auto toCopySource = Transition(
                computeOutput.Get(),
                D3D12_RESOURCE_STATE_COPY_DEST,
                D3D12_RESOURCE_STATE_COPY_SOURCE);
            list->ResourceBarrier(1, &toCopySource);
        });
    auto computeCopy = manager.Submit(computeCopyRecipe, { directWrite });

    auto directReadbackRecipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        [computeOutput, readback](ID3D12GraphicsCommandList* list) {
            list->CopyBufferRegion(readback.Get(), 0, computeOutput.Get(), 0, sizeof(std::uint32_t));
        });
    auto directReadback = manager.Submit(directReadbackRecipe, { computeCopy });
    directReadback->WaitForExecute();

    directWrite->FutureWait();
    computeCopy->FutureWait();
    passed &= Check(directWrite->GetFenceValue() != 0, "first direct submission must not use completed fence zero");
    passed &= Check(computeCopy->GetFenceValue() != 0, "first compute submission must not use completed fence zero");
    passed &= Check(
        directReadback->GetFenceValue() > directWrite->GetFenceValue(),
        "direct queue submission tickets must increase monotonically");
    passed &= Check(directReadback->IsDone(), "final direct submission must complete");

    void* readbackData = nullptr;
    const D3D12_RANGE readRange{ 0, sizeof(std::uint32_t) };
    if (CheckHR(readback->Map(0, &readRange, &readbackData), "Map readback buffer")) {
        std::uint32_t actualValue = 0;
        std::memcpy(&actualValue, readbackData, sizeof(actualValue));
        readback->Unmap(0, nullptr);
        passed &= Check(
            actualValue == expectedValue,
            "direct -> compute -> direct queue dependency chain must preserve copied data");
    }
    else {
        passed = false;
    }

    manager.WaitIdle();
    auto* directQueue = manager.GetQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    auto* computeQueue = manager.GetQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE);
    const auto directCompletedAfterIdle = directQueue->GetFence()->GetCompletedValue();
    const auto computeCompletedAfterIdle = computeQueue->GetFence()->GetCompletedValue();

    auto directAfterIdleRecipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        [](ID3D12GraphicsCommandList*) {});
    auto directAfterIdle = manager.Submit(directAfterIdleRecipe);
    directAfterIdle->FutureWait();
    passed &= Check(
        directAfterIdle->GetFenceValue() > directCompletedAfterIdle,
        "submission after WaitIdle must receive a non-completed direct-queue ticket");

    auto computeAfterIdleRecipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(
        D3D12_COMMAND_LIST_TYPE_COMPUTE,
        [](ID3D12GraphicsCommandList*) {});
    auto computeAfterIdle = manager.Submit(computeAfterIdleRecipe, { directAfterIdle });
    computeAfterIdle->FutureWait();
    passed &= Check(
        computeAfterIdle->GetFenceValue() > computeCompletedAfterIdle,
        "submission after WaitIdle must receive a non-completed compute-queue ticket");
    passed &= Check(
        computeAfterIdle->GetFenceValue() > computeCopy->GetFenceValue(),
        "compute queue submission tickets must increase monotonically");
    computeAfterIdle->WaitForExecute();

    manager.Shutdown();

    if (!passed) return 1;
    std::cout << "Render queue dependency tests passed\n";
    return 0;
}
