#include <UI/MainRenderer_UIComponent.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_dx12.h>
#include <backends/imgui_impl_win32.h>


void MainRenderer_UIComponent::InitImGui(ID3D12Device* device, const HWND& hwnd, ID3D12DescriptorHeap* srvHeap) {
    IMGUI_CHECKVERSION();

    ImGui::CreateContext();

    ImGui_ImplWin32_Init(hwnd);

    ImGuiIO& io = ImGui::GetIO();
    io.MouseDrawCursor = true;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplDX12_Init(
        device,
        BUFFERS_COUNT,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        srvHeap,
        srvHeap->GetCPUDescriptorHandleForHeapStart(),
        srvHeap->GetGPUDescriptorHandleForHeapStart()
    );

    ImGui::StyleColorsDark();

    m_bIsInited = true;
}

void MainRenderer_UIComponent::RenderImGui(ID3D12GraphicsCommandList* cmdList) {
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    DrawUI();

    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);
}

void MainRenderer_UIComponent::ShutDownImGui() {
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void MainRenderer_UIComponent::ImGuiInputProcess(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
}

void MainRenderer_UIComponent::DrawUI() {
    ImGui::Begin("Hello, world!");
    ImGui::End();
}