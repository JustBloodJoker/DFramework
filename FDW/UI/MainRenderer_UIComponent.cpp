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

//TODO: Implement UI parsing from external format (e.g., JSON or XML) to define layout/widgets
void MainRenderer_UIComponent::DrawUI() {
    MainWindow();
    SceneBrowser();
}

void MainRenderer_UIComponent::MainWindow() {
    ImGui::Begin("Main WND");

    if (ImGui::Button("Load Scene")) {
        m_bShowSceneBrowser = true;
    }

    ElementParamSetter();
    
    ImGui::End();
}

void MainRenderer_UIComponent::SceneBrowser() {
    if (!m_bShowSceneBrowser) return;

    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
    ImGui::Begin("Scene Browser", &m_bShowSceneBrowser, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

    if (bPathChanged) {
        entries = GetDirectoryEntries(currentPath);
        bPathChanged = false;
    }

    if (ImGui::Button("Back")) {
        if (currentPath.has_parent_path()) {
            currentPath = currentPath.parent_path();
            bPathChanged = true;
        }
    }

    ImGui::Separator();

    for (const auto& entry : entries) {
        if (!entry.exists()) continue;

        std::string label = WStringToUTF8(entry.path().filename().wstring());

        bool selected = false;
        if (entry.is_directory()) {
            selected = ImGui::Selectable(("Folder " + label).c_str());
            if (selected) {
                currentPath /= entry.path().filename();
                bPathChanged = true;
            }
        }
        else if (entry.is_regular_file() && entry.path().extension() == L".gltf") {
            selected = ImGui::Selectable(("File " + label).c_str());
            if (selected) {
                m_pOwner->AddScene(entry.path().string());
                m_bShowSceneBrowser = false;
            }
        }

        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("Name: %s", label.c_str());
            ImGui::Text("Path: %s", WStringToUTF8(entry.path().wstring()).c_str());

            if (entry.is_regular_file()) {
                std::error_code ec;
                auto fsize = std::filesystem::file_size(entry.path(), ec);
                auto ftime = std::filesystem::last_write_time(entry.path(), ec);
                if (!ec) {
                    std::time_t cftime = std::chrono::system_clock::to_time_t(
                        std::chrono::clock_cast<std::chrono::system_clock>(ftime));
                    std::tm tmResult{};
                    char timeStr[26] = {};
                    if (localtime_s(&tmResult, &cftime) == 0) {
                        asctime_s(timeStr, sizeof(timeStr), &tmResult);
                        ImGui::Text("Size: %.2f KB", fsize / 1024.0f);
                        ImGui::Text("Modified: %s", timeStr);
                    }
                }
            }
            ImGui::EndTooltip();
        }
    }

    ImGui::Separator();
    if (ImGui::Button("Close")) {
        m_bShowSceneBrowser = false;
    }

    ImGui::End();
}

void MainRenderer_UIComponent::ElementParamSetter() {
    m_vCachedObjects = m_pOwner->GetRenderableObjects();
    if (!m_vCachedObjects.empty()) {
        ImGui::Separator();
        ImGui::Text("Scene Objects");

        std::vector<const char*> names;
        names.reserve(m_vCachedObjects.size());
        for (const auto* obj : m_vCachedObjects) {
            names.push_back(obj->GetName().c_str());
        }

        if (m_iSelectedObjectIndex >= (int)m_vCachedObjects.size())
            m_iSelectedObjectIndex = -1;

        ImGui::Combo("Select Object", &m_iSelectedObjectIndex, names.data(), (int)names.size());

        if (m_iSelectedObjectIndex >= 0) {
            auto selectedObj = m_vCachedObjects[m_iSelectedObjectIndex];

            if (auto* mesh = dynamic_cast<RenderableMesh*>(selectedObj)) {
                const auto& anims = mesh->GetAnimations();
                if (!anims.empty()) {
                    auto& animState = m_mAnimationStates[selectedObj];

                    ImGui::Separator();
                    ImGui::Text("Animations");

                    std::vector<const char*> animNames;
                    animNames.reserve(anims.size());
                    for (const auto& anim : anims)
                        animNames.push_back(anim.c_str());

                    if (animState.SelectedAnimIndex >= (int)anims.size())
                        animState.SelectedAnimIndex = 0;

                    ImGui::Combo("Animation", &animState.SelectedAnimIndex, animNames.data(), (int)animNames.size());

                    if (ImGui::Button("Play")) {
                        mesh->PlayAnimation(anims[animState.SelectedAnimIndex]);
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("Stop")) {
                        mesh->StopAnimation();
                    }

                    if (ImGui::Checkbox("Freeze", &animState.IsFreeze)) {
                        mesh->FreezeAnimation(animState.IsFreeze);
                    }

                    ImGui::Separator();
                }
            }

            dx::XMFLOAT3 pos = selectedObj->GetPosition();
            dx::XMFLOAT3 rot = selectedObj->GetRotation();
            dx::XMFLOAT3 scale = selectedObj->GetScale();

            ImGui::Separator();
            ImGui::Text("Transform");

            if (ImGui::DragFloat3("Position", (float*)&pos, 0.1f)) {
                selectedObj->SetPosition(pos);
            }

            if (ImGui::DragFloat3("Rotation", (float*)&rot, 1.0f)) {
                selectedObj->SetRotation(rot);
            }

            if (ImGui::DragFloat3("Scale", (float*)&scale, 0.05f)) {
                selectedObj->SetScale(scale);
            }

            if (auto* mesh = dynamic_cast<RenderableMesh*>(selectedObj)) {
                if (ImGui::CollapsingHeader("Mesh Elements", ImGuiTreeNodeFlags_DefaultOpen)) {
                    auto elements = mesh->GetRenderableElements();
                    for (size_t i = 0; i < elements.size(); ++i) {
                        auto* element = elements[i];
                        std::string label = "Element " + std::to_string(i);

                        if (ImGui::TreeNode(label.c_str())) {
                            dx::XMFLOAT3 ePos = element->GetPosition();
                            dx::XMFLOAT3 eRot = element->GetRotation();
                            dx::XMFLOAT3 eScale = element->GetScale();

                            if (ImGui::DragFloat3("Pos", (float*)&ePos, 0.1f)) {
                                element->SetPosition(ePos);
                            }
                            if (ImGui::DragFloat3("Rot", (float*)&eRot, 1.0f)) {
                                element->SetRotation(eRot);
                            }
                            if (ImGui::DragFloat3("Scale", (float*)&eScale, 0.05f)) {
                                element->SetScale(eScale);
                            }

                            ImGui::TreePop();
                        }
                    }
                }
            }
        }
    }
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


std::vector<std::filesystem::path> MainRenderer_UIComponent::FindSceneFiles(const std::wstring& root, const std::vector<std::wstring>& extensions) {
    std::vector<std::filesystem::path> result;
    for (auto& p : std::filesystem::recursive_directory_iterator(root)) {
        if (!p.is_regular_file()) continue;
        auto ext = p.path().extension().wstring();
        for (const auto& wanted : extensions) {
            if (ext == wanted) {
                result.push_back(p.path());
                break;
            }
        }
    }
    return result;
}

std::vector<std::filesystem::directory_entry> MainRenderer_UIComponent::GetDirectoryEntries(const std::filesystem::path& dir) {
    std::vector<std::filesystem::directory_entry> result;
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
        if (ec) continue;
        result.push_back(entry);
    }

    std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) {
        if (a.is_directory() != b.is_directory())
            return a.is_directory() > b.is_directory();
        return a.path().filename().wstring() < b.path().filename().wstring();
        });

    return result;
}

std::string MainRenderer_UIComponent::WStringToUTF8(const std::wstring& wstr) {
    if (wstr.empty()) return {};

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
    if (size_needed <= 0 || size_needed > 1024 * 1024)
        return "<invalid path>";

    std::string result(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), &result[0], size_needed, nullptr, nullptr);
    return result;
}