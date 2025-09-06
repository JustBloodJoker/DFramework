#include <UI/MainRenderer_UIComponent.h>
#include <UI/UIInputLayer.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_dx12.h>
#include <backends/imgui_impl_win32.h>
#include <MainRenderer/MainRenderer.h>
#include <D3DFramework/GraphicUtilites/ResourcePacker.h>

#include <World/World.h>

/////////////////////////////////////////
///         ---ENGINE_UI---


static const std::vector<std::wstring> s_vSupportedSceneExts = { L".gltf", L".fbx" };
static const std::vector<std::wstring> s_vSupportedSkyboxExts = { L".hdr", L".dds", L".skybox" };
static const std::vector<std::wstring> s_vSupportedAudioExts = { L".wav" };
static const std::vector<std::wstring> s_vSupportedTextureExts = { L".jpg", L".png" };

static const std::vector<std::wstring> s_vSupportedEngineFileExts = { L".fdw" };

//TODO: Implement UI parsing from external format (e.g., JSON or XML) to define layout/widgets
void MainRenderer_UIComponent::DrawUI() {
    MainWindow();
    SceneBrowser();
    AudioBrowser();
    SkyboxBrowser();
    TextureBrowser();
    LoadSceneBrowser();
    SaveSceneBrowser();
}

void MainRenderer_UIComponent::MainWindow() {
    ImGui::Begin("Main WND");

    if (ImGui::BeginTabBar("MainTabs")) {

        if (ImGui::BeginTabItem("Renderable Objects")) {
            DrawRenderableObjectsTab();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Camera")) {
            DrawCameraTab();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Lights")) {
            DrawLightsTab();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Shadows")) {
            DrawShadowsTab();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("EditorUtilities")) {
            DrawEditorUtilitiesTab();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

void MainRenderer_UIComponent::DrawRenderableObjectsTab() {
    /*if (ImGui::Button("Load Scene")) {
        m_bShowSceneBrowser = true;
    }
    if (ImGui::Button("Load Audio")) {
        m_bShowAudioBrowser = true;
    }
    if (ImGui::Button("Load Skybox")) {
        m_bShowSkyboxBrowser = true;
    }*/
}

void MainRenderer_UIComponent::DrawCameraTab() {
    ImGui::SeparatorText("Cameras");
    //TODO CAMERA TAB
    //Creation
    //TBaseCamera
}

void MainRenderer_UIComponent::DrawShadowsTab() {
    ImGui::SeparatorText("Shadow Settings");
    
    bool changed = false;
    if (m_pOwner->IsShadowEnabled()) {
        RTShadowSystemConfig config = m_pOwner->GetRTShadowConfig();

        changed |= ImGui::SliderFloat("Temporal Feedback Min", &config.TemporalFeedbackMin, 0.0f, 1.0f);
        changed |= ImGui::SliderFloat("Temporal Feedback Max", &config.TemporalFeedbackMax, 0.0f, 1.0f);

        changed |= ImGui::SliderFloat("Reprojection Distance Threshold", &config.ReprojDistThreshold, 0.0f, 1.0f);
        changed |= ImGui::SliderFloat("Normal Threshold", &config.NormalThreshold, 0.0f, 1.0f);

        changed |= ImGui::SliderFloat("Sigma Spatial (SigmaS)", &config.SigmaS, 0.1f, 10.0f);
        changed |= ImGui::SliderFloat("Sigma Range (SigmaR)", &config.SigmaR, 0.0f, 1.0f);
        changed |= ImGui::SliderInt("Kernel Radius", &config.KernelRadius, 1, 10);

        if (changed) {
            m_pOwner->SetRTShadowConfig(config);
        }
    }
    else {
        ImGui::TextDisabled("NotImpl");
    }
}

void MainRenderer_UIComponent::DrawLightsTab() {
    ImGui::SeparatorText("Lights");
    //TODO lights tab
    //creation


}

void MainRenderer_UIComponent::DrawEditorUtilitiesTab() {
    ImGui::SeparatorText("Scene culling");

    auto current = m_pOwner->GetMeshCullingType();
    const char* cullingNames[] = { "None", "GPU Culling" };
    int currentIndex = static_cast<int>(current);
    if (ImGui::Combo("Culling Type", &currentIndex, cullingNames, IM_ARRAYSIZE(cullingNames))) {
        m_pOwner->SetMeshCullingType(static_cast<MeshCullingType>(currentIndex));
    }

    auto bb = m_pOwner->IsEnabledPreDepth();
    if (ImGui::Checkbox("Enable Pre Depth pass", &bb)) {
        m_pOwner->EnablePreDepth(bb);
    }

    ImGui::SeparatorText("Scene I/O");

    if (ImGui::Button("Save Scene")) {
        m_bShowSaveSceneBrowser = true;
    }

    if (ImGui::Button("Load Scene")) {
        m_bShowLoadSceneBrowser = true;
    }
}


void MainRenderer_UIComponent::SceneBrowser() {
    FileBrowser(
        m_bShowSceneBrowser,
        "Scene Browser",
        m_xCurrentPath,
        s_vSupportedSceneExts,
        [this](const std::filesystem::path& path) {
            //todo add scene
        },
        [](const std::filesystem::path& path) {
            return "Scene ";
        }
    );
}

void MainRenderer_UIComponent::AudioBrowser() {
    FileBrowser(
        m_bShowAudioBrowser,
        "Audio Browser",
        m_xCurrentPath,
        s_vSupportedAudioExts,
        [this](const std::filesystem::path& path) {
           //todo add audio
        },
        [](const std::filesystem::path& path) {
            return "Audio ";
        }
    );
}

void MainRenderer_UIComponent::SkyboxBrowser() {
    FileBrowser(
        m_bShowSkyboxBrowser,
        "Skybox Browser",
        m_xCurrentPath,
        s_vSupportedSkyboxExts,
        [this](const std::filesystem::path& path) {
            //todo add skybox
        },
        [](const std::filesystem::path& path) {
            return "Skybox ";
        }
    );
}

void MainRenderer_UIComponent::TextureBrowser() {
    FileBrowser(
        m_bShowTextureBrowser,
        "Texture Browser",
        m_xCurrentPath,
        s_vSupportedTextureExts,
        [this](const std::filesystem::path& path) {
           //todo set texture for simple objects
        },
        [](const std::filesystem::path& path) {
            return "Texture ";
        }
    );
}

void MainRenderer_UIComponent::SaveSceneBrowser() {
    SceneFileBrowser(m_bShowSaveSceneBrowser, "Save Scene", true,
        [this](const std::filesystem::path& path) {
            AddCallToPull([this, path]() {m_pOwner->SaveActiveWorld(path.string()); });
    });
}

void MainRenderer_UIComponent::LoadSceneBrowser() {
    SceneFileBrowser(m_bShowLoadSceneBrowser, "Load Scene", false,
        [this](const std::filesystem::path& path) {
            AddCallToPull([this, path]() {m_pOwner->LoadWorld(path.string()); });
    });
}

/////////////////////////////////////////


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

void MainRenderer_UIComponent::RenderImGui(ID3D12GraphicsCommandList* list) {
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    
    DrawUI();

    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), list);
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


void MainRenderer_UIComponent::AddCallToPull(std::function<void(void)> foo) {
    if (foo)m_vCallsAfterRender.push_back(foo);
}

void MainRenderer_UIComponent::ProcessAfterRenderUICalls() {
    auto vv = m_vCallsAfterRender;
    m_vCallsAfterRender.clear();
    for (auto foo : vv) {
        if (foo) foo();
    }
}

void MainRenderer_UIComponent::AfterConstruction() {
    static const std::pair<const char*, FD3DW::TextureType> kTextureNames[] = {
        { "BASE",     FD3DW::TextureType::BASE },
        { "NORMAL",   FD3DW::TextureType::NORMAL },
        { "ROUGHNESS",FD3DW::TextureType::ROUGHNESS },
        { "METALNESS",FD3DW::TextureType::METALNESS },
        { "HEIGHT",   FD3DW::TextureType::HEIGHT },
        { "SPECULAR", FD3DW::TextureType::SPECULAR },
        { "OPACITY",  FD3DW::TextureType::OPACITY },
        { "AMBIENT",    FD3DW::TextureType::AMBIENT },
        { "EMISSIVE", FD3DW::TextureType::EMISSIVE }
    };

    for (const auto& [name, type] : kTextureNames)
    {
        m_vTextureOps.push_back({
            name,
            [this, type](TSimpleMesh* obj, const std::string& path) {
                auto hh = GlobalRenderThreadManager::GetInstance()->CreateWaitHandle(D3D12_COMMAND_LIST_TYPE_DIRECT);
                auto recipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_COPY, [this, obj, type, path](ID3D12GraphicsCommandList* list) {
                    obj->SetupTexture(type, path, m_pOwner->GetDevice(), list);
                });
                GlobalRenderThreadManager::GetInstance()->Submit(recipe, { hh });
            },
            [this, type](TSimpleMesh* obj) {
                obj->EraseTexture(type, m_pOwner->GetDevice());
            }
            });
    }

    InitImGui();

    m_pUILayer = std::make_unique<UIInputLayer>(this);
    m_pUILayer->AddToRouter( m_pOwner->GetInputRouter() );
}

void MainRenderer_UIComponent::BeforeDestruction() {
    ShutDownImGui();

    m_pUILayer->AddToRouter(nullptr);
}


void MainRenderer_UIComponent::SceneFileBrowser(bool& isOpen, const std::string& title, bool isSave, const std::function<void(const std::filesystem::path&)>& onConfirm) {
    if (!isOpen) return;

    static char fileNameBuffer[256] = {};


    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
    ImGui::Begin(title.c_str(), &isOpen, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

    if (m_bIsPathChanged) {
        m_vEntries = GetDirectoryEntries(m_xCurrentPath);
        m_bIsPathChanged = false;
    }

    if (ImGui::Button("Back")) {
        if (m_xCurrentPath.has_parent_path()) {
            m_xCurrentPath = m_xCurrentPath.parent_path();
            m_bIsPathChanged = true;
        }
    }

    ImGui::Separator();

    for (const auto& entry : m_vEntries) {
        if (!entry.exists()) continue;
        const auto& path = entry.path();
        std::wstring ext = path.extension().wstring();
        std::string label = WStringToUTF8(path.filename().wstring());

        if (entry.is_directory()) {
            if (ImGui::Selectable(("Folder " + label).c_str())) {
                m_xCurrentPath /= path.filename();
                m_bIsPathChanged = true;
            }
        }
        else if (!isSave && std::find(s_vSupportedEngineFileExts.begin(), s_vSupportedEngineFileExts.end(), ext) != s_vSupportedEngineFileExts.end()) {
            if (ImGui::Selectable(("Scene " + label).c_str())) {
                onConfirm(path);
                isOpen = false;
                break;
            }
        }
    }

    if (isSave) {
        ImGui::SeparatorText("Save Scene As...");
        ImGui::InputText("File name", fileNameBuffer, sizeof(fileNameBuffer));
        ImGui::SameLine();

        if (ImGui::Button("Save")) {
            std::string inputName(fileNameBuffer);
            if (!inputName.empty()) {
                auto fullPath = m_xCurrentPath / inputName;
                if (fullPath.extension().empty()) {
                    fullPath += ".fdw";
                }
                onConfirm(fullPath);
                isOpen = false;
            }
        }
    }

    ImGui::Separator();
    if (ImGui::Button("Close")) {
        isOpen = false;
    }

    ImGui::End();
}

void MainRenderer_UIComponent::FileBrowser(
    bool& isOpen,
    const std::string& windowTitle,
    std::filesystem::path& currentPath,
    const std::vector<std::wstring>& supportedExtensions,
    const std::function<void(const std::filesystem::path&)>& onFileSelected,
    const std::function<std::string(const std::filesystem::path&)>& getIcon
) {
    if (!isOpen) return;

    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
    ImGui::Begin(windowTitle.c_str(), &isOpen, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

    if (m_bIsPathChanged) {
        m_vEntries = GetDirectoryEntries(currentPath);
        m_bIsPathChanged = false;
    }

    if (ImGui::Button("Back")) {
        if (currentPath.has_parent_path()) {
            currentPath = currentPath.parent_path();
            m_bIsPathChanged = true;
        }
    }

    ImGui::Separator();

    for (const auto& entry : m_vEntries) {
        if (!entry.exists()) continue;

        const auto& path = entry.path();
        std::wstring ext = path.extension().wstring();
        std::string label = WStringToUTF8(path.filename().wstring());

        bool selected = false;

        if (entry.is_directory()) {
            selected = ImGui::Selectable(("Folder " + label).c_str());
            if (selected) {
                currentPath /= path.filename();
                m_bIsPathChanged = true;
            }
        }
        else if (entry.is_regular_file()) {
            if (!supportedExtensions.empty() && std::find(supportedExtensions.begin(), supportedExtensions.end(), ext) == supportedExtensions.end())
                continue;

            std::string icon = getIcon ? getIcon(path) : "File ";
            selected = ImGui::Selectable((icon + label).c_str());

            if (selected && onFileSelected) {
                onFileSelected(path);
                isOpen = false;
                break;
            }
        }

        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("Name: %s", label.c_str());
            ImGui::Text("Path: %s", WStringToUTF8(path.wstring()).c_str());

            if (entry.is_regular_file()) {
                std::error_code ec;
                auto fsize = std::filesystem::file_size(path, ec);
                auto ftime = std::filesystem::last_write_time(path, ec);
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
        isOpen = false;
    }

    ImGui::End();
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