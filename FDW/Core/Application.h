//// Application.h
//#pragma once
//#include <D3DFramework/D3DFW.h>
//#include "UI/UIManager.h"
//#include "Scene/SceneManager.h"
//#include "Renderer/RenderManager.h"
//#include "Resources/ResourceManager.h"
//
//class Application : public FD3DW::D3DFW {
//public:
//    Application();
//    void UserInit() override;
//    void UserLoop() override;
//    void UserClose() override;
//    void UserResizeUpdate() override;
//    void ChildAllMSG(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
//
//private:
//    std::unique_ptr<UIManager> m_UIManager;
//    std::unique_ptr<SceneManager> m_SceneManager;
//    std::unique_ptr<RenderManager> m_RenderManager;
//    std::unique_ptr<ResourceManager> m_ResourceManager;
//};
//
//// Application.cpp
//#include "Application.h"
//
//Application::Application() : D3DFW(L"MyEngine", 1280, 720, false) {}
//
//void Application::UserInit() {
//    m_ResourceManager = std::make_unique<ResourceManager>(GetDevice());
//    m_SceneManager = std::make_unique<SceneManager>(m_ResourceManager.get());
//    m_RenderManager = std::make_unique<RenderManager>(this, m_SceneManager.get());
//    m_UIManager = std::make_unique<UIManager>(this, m_SceneManager.get(), m_RenderManager.get());
//}
//
//void Application::UserLoop() {
//    auto* cmd = GetCommandList();
//    m_SceneManager->Update();
//    m_RenderManager->RenderFrame(GetCurrBackBufferView(), cmd, GetMainViewPort(), GetMainRect());
//    m_UIManager->RenderUI(cmd);
//}
//
//void Application::UserClose() {
//    m_UIManager->Shutdown();
//}
//
//void Application::UserResizeUpdate() {
//    m_RenderManager->Resize(GetMainWNDSettings());
//}
//
//void Application::ChildAllMSG(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
//    m_UIManager->HandleInput(hWnd, msg, wParam, lParam);
//}
//
//// UIManager.cpp
//#include "UIManager.h"
//#include "Scene/SceneManager.h"
//#include "Renderer/RenderManager.h"
//#include <imgui.h>
//#include <backends/imgui_impl_dx12.h>
//#include <backends/imgui_impl_win32.h>
//
//UIManager::UIManager(Application* app, SceneManager* scene, RenderManager* render)
//    : m_App(app), m_Scene(scene), m_Render(render) {
//    IMGUI_CHECKVERSION();
//    ImGui::CreateContext();
//    ImGuiIO& io = ImGui::GetIO();
//    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
//    ImGui::StyleColorsDark();
//    ImGui_ImplWin32_Init(app->GETHWND());
//    ImGui_ImplDX12_Init(
//        app->GetDevice(),
//        BUFFERS_COUNT,
//        DXGI_FORMAT_R8G8B8A8_UNORM,
//        app->GetUISRVHeap(),
//        app->GetUISRVHeap()->GetCPUDescriptorHandleForHeapStart(),
//        app->GetUISRVHeap()->GetGPUDescriptorHandleForHeapStart()
//    );
//    m_bInited = true;
//}
//
//void UIManager::RenderUI(ID3D12GraphicsCommandList* cmd) {
//    ImGui_ImplDX12_NewFrame();
//    ImGui_ImplWin32_NewFrame();
//    ImGui::NewFrame();
//    DrawUI();
//    ImGui::Render();
//    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmd);
//}
//
//void UIManager::Shutdown() {
//    ImGui_ImplDX12_Shutdown();
//    ImGui_ImplWin32_Shutdown();
//    ImGui::DestroyContext();
//}
//
//void UIManager::HandleInput(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
//    ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam);
//}
//
//void UIManager::DrawUI() {
//    ImGui::Begin("Scene Controls");
//
//    static char pathBuffer[255] = "";
//    ImGui::InputText("Scene Path", pathBuffer, sizeof(pathBuffer));
//    if (ImGui::Button("Add Scene")) {
//        m_Scene->LoadScene(pathBuffer);
//    }
//
//    ImGui::End();
//}
//
//// SceneManager.cpp
//#include "SceneManager.h"
//#include "Resources/ResourceManager.h"
//#include <D3DFramework/Objects/Scene.h>
//
//SceneManager::SceneManager(ResourceManager* resMgr)
//    : m_ResMgr(resMgr) {}
//
//void SceneManager::AddCube() {
//    // TODO: Add default cube logic here if needed
//}
//
//void SceneManager::Update() {
//    // TODO: Implement per-frame updates
//}
//
//void SceneManager::LoadScene(const std::string& path) {
//    auto scene = m_ResMgr->GetFramework()->CreateScene(path, true, m_ResMgr->GetCommandList());
//    m_Objects.push_back(scene);
//}
//
//const std::vector<std::shared_ptr<Object>>& SceneManager::GetObjects() const {
//    return m_Objects;
//}
//
//// RenderManager.cpp
//#include "RenderManager.h"
//#include "Scene/SceneManager.h"
//#include <D3DFramework/Objects/Object.h>
//
//RenderManager::RenderManager(FD3DW::D3DFW* framework, SceneManager* sceneMgr)
//    : m_Framework(framework), m_SceneMgr(sceneMgr) {
//    // TODO: Initialize PSO/RootSignature from your shader setup
//}
//
//void RenderManager::RenderFrame(D3D12_CPU_DESCRIPTOR_HANDLE rtv, ID3D12GraphicsCommandList* cmdList, D3D12_VIEWPORT vp, D3D12_RECT rect) {
//    FLOAT bgColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
//    cmdList->RSSetViewports(1, &vp);
//    cmdList->RSSetScissorRects(1, &rect);
//    cmdList->ClearRenderTargetView(rtv, bgColor, 0, nullptr);
//    cmdList->OMSetRenderTargets(1, &rtv, TRUE, nullptr);
//
//    for (auto& obj : m_SceneMgr->GetObjects()) {
//        if (obj) obj->Render(cmdList);
//    }
//}
//
//void RenderManager::Resize(const FD3DW::WindowSettings& settings) {
//    // Update viewport/RTV/etc. if needed
//}
//
//// ResourceManager.cpp
//#include "ResourceManager.h"
//#include <D3DFramework/CommandQueue.h>
//
//ResourceManager::ResourceManager(ID3D12Device* device)
//    : m_Device(device) {
//    m_CommandList = std::make_unique<FD3DW::CommandList>(D3D12_COMMAND_LIST_TYPE_DIRECT, device);
//}
//
//ID3D12Device* ResourceManager::GetDevice() const {
//    return m_Device;
//}
//
//ID3D12GraphicsCommandList* ResourceManager::GetCommandList() const {
//    return m_CommandList->GetList();
//}
//
//FD3DW::D3DFW* ResourceManager::GetFramework() const {
//    return static_cast<FD3DW::D3DFW*>(FD3DW::D3DFW::GetInstance());
//}
