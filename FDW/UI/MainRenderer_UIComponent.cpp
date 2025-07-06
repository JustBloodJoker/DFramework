#include <UI/MainRenderer_UIComponent.h>
#include <UI/UIInputLayer.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_dx12.h>
#include <backends/imgui_impl_win32.h>
#include <MainRenderer/MainRenderer.h>
#include <D3DFramework/GraphicUtilites/ResourcePacker.h>


/////////////////////////////////////////
///         ---ENGINE_UI---

void MainRenderer_UIComponent::DrawUI() {
    ImGui::Begin("Hello, world!");



    ImGui::End();
}

/////////////////////////////////////////



MainRenderer_UIComponent::MainRenderer_UIComponent(MainRenderer* owner) : MainRendererComponent(owner) { 
}

void MainRenderer_UIComponent::InitImGui() {
    if (m_bIsInited) return;
    
    m_pUISRVPack = m_pOwner->CreateSRVPack(1u);
    
    IMGUI_CHECKVERSION();

    ImGui::CreateContext();

    ImGui_ImplWin32_Init( m_pOwner->GETHWND() );

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    auto heap = m_pUISRVPack->GetResult()->GetDescriptorPtr();
    ImGui_ImplDX12_Init(
        m_pOwner->GetDevice(),
        BUFFERS_COUNT,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        heap,
        heap->GetCPUDescriptorHandleForHeapStart(),
        heap->GetGPUDescriptorHandleForHeapStart()
    );

    ImGui::StyleColorsDark();

    m_bIsInited = true;
}

void MainRenderer_UIComponent::RenderImGui() {
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    DrawUI();

    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_pOwner->GetBindedCommandList());
}

void MainRenderer_UIComponent::ShutDownImGui() {
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    m_pUISRVPack.release();
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

bool MainRenderer_UIComponent::ImGuiInputProcess(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
    
    return false;
}

void MainRenderer_UIComponent::AfterConstruction() {
    InitImGui();

    m_pUILayer = std::make_unique<UIInputLayer>(this);
    m_pUILayer->AddToRouter( m_pOwner->GetInputRouter() );
}

void MainRenderer_UIComponent::BeforeDestruction() {
    ShutDownImGui();

    m_pUILayer->AddToRouter(nullptr);
}
