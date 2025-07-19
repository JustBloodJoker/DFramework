#include <UI/MainRenderer_UIComponent.h>
#include <UI/UIInputLayer.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_dx12.h>
#include <backends/imgui_impl_win32.h>
#include <MainRenderer/MainRenderer.h>
#include <D3DFramework/GraphicUtilites/ResourcePacker.h>

#include <RenderableObjects/RenderableMesh.h>
#include <RenderableObjects/RenderableMeshElement.h>
#include <RenderableObjects/RenderableSimpleObject.h>
#include <RenderableObjects/RenderableSkyboxObject.h>
#include <RenderableObjects/RenderableAudioObject.h>

/////////////////////////////////////////
///         ---ENGINE_UI---


static const std::vector<std::wstring> s_vSupportedSceneExts = { L".gltf", L".fbx" };
static const std::vector<std::wstring> s_vSupportedSkyboxExts = { L".hdr", L".dds", L".skybox" };
static const std::vector<std::wstring> s_vSupportedAudioExts = { L".wav" };
static const std::vector<std::wstring> s_vSupportedTextureExts = { L".jpg", L".png" };

//TODO: Implement UI parsing from external format (e.g., JSON or XML) to define layout/widgets
void MainRenderer_UIComponent::DrawUI() {
    MainWindow();
    SceneBrowser();
    AudioBrowser();
    SkyboxBrowser();
    TextureBrowser();
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

        ImGui::EndTabBar();
    }

    ImGui::End();
}

void MainRenderer_UIComponent::DrawRenderableObjectsTab() {
    if (ImGui::Button("Load Scene")) {
        m_bShowSceneBrowser = true;
    }
    if (ImGui::Button("Load Audio")) {
        m_bShowAudioBrowser = true;
    }
    if (ImGui::Button("Load Skybox")) {
        m_bShowSkyboxBrowser = true;
    }
    if (ImGui::Button("Create Simple Plane")) {
        m_pOwner->AddSimplePlane();
    }
    if (m_iSelectedObjectIndex >= 0 && m_iSelectedObjectIndex < (int)m_vCachedObjects.size()) {
        ImGui::SameLine();
        if (ImGui::Button("Delete Selected")) {
            auto* selectedObj = m_vCachedObjects[m_iSelectedObjectIndex];
            m_pOwner->RemoveObject(selectedObj);
            m_iSelectedObjectIndex = -1;
        }
    }
    ElementParamSetter();
}

void MainRenderer_UIComponent::DrawCameraTab() {
    ImGui::SeparatorText("Camera Controls");

    float moveSpeed = m_pOwner->GetCameraSpeed();
    if (ImGui::SliderFloat("Move Speed", &moveSpeed, 0.1f, 2000.0f, "%.2f")) {
        m_pOwner->SetCameraSpeed(moveSpeed);
    }

    ImGui::Spacing();
    ImGui::Spacing();

    ImGui::SeparatorText("Actions");
    if (ImGui::Button("Reset Position")) {
        m_pOwner->SetDefaultPosition();
    }
}

void MainRenderer_UIComponent::SceneBrowser() {
    FileBrowser(
        m_bShowSceneBrowser,
        "Scene Browser",
        currentPath,
        s_vSupportedSceneExts,
        [this](const std::filesystem::path& path) {
            m_pOwner->AddScene(path.string());
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
        currentPath,
        s_vSupportedAudioExts,
        [this](const std::filesystem::path& path) {
            m_pOwner->AddAudio(path.string());
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
        currentPath,
        s_vSupportedSkyboxExts,
        [this](const std::filesystem::path& path) {
            m_pOwner->AddSkybox(path.string());
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
        currentPath,
        s_vSupportedTextureExts,
        [this](const std::filesystem::path& path) {
            if (m_pTextureTargetObject && m_iTextureBeingSet >= 0 && m_iTextureBeingSet < std::size(m_vTextureOps)) {
                m_vTextureOps[m_iTextureBeingSet].SetFunc(m_pTextureTargetObject, path.string());
            }
        },
        [](const std::filesystem::path& path) {
            return "Texture ";
        }
    );
}

void MainRenderer_UIComponent::ElementParamSetter() {
    m_vCachedObjects = m_pOwner->GetRenderableObjects();
    if (m_vCachedObjects.empty()) return;

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

    if (m_iSelectedObjectIndex < 0) return;

    BaseRenderableObject* selectedObj = m_vCachedObjects[m_iSelectedObjectIndex];

    if (auto* mesh = dynamic_cast<RenderableMesh*>(selectedObj)) {
        DrawMeshUI(mesh);
    }
    else if (auto* simple = dynamic_cast<RenderableSimpleObject*>(selectedObj)) {
        DrawSimpleRenderableUI(simple);
    }
    else if (auto* skybox = dynamic_cast<RenderableSkyboxObject*>(selectedObj)) {
        DrawSkyboxUI(skybox);
    }
    else if (auto* audio = dynamic_cast<RenderableAudioObject*>(selectedObj)) {
        DrawAudioUI(audio);
    }
}

void MainRenderer_UIComponent::DrawMeshUI(RenderableMesh* mesh) {
    ImGui::Separator();
    ImGui::Text("Transform");

    dx::XMFLOAT3 pos = mesh->GetPosition();
    dx::XMFLOAT3 rot = mesh->GetRotation();
    dx::XMFLOAT3 scale = mesh->GetScale();

    if (ImGui::DragFloat3("Position", (float*)&pos, 0.1f)) {
        mesh->SetPosition(pos);
    }

    if (ImGui::DragFloat3("Rotation", (float*)&rot, 1.0f)) {
        mesh->SetRotation(rot);
    }

    if (ImGui::DragFloat3("Scale", (float*)&scale, 0.05f)) {
        mesh->SetScale(scale);
    }

    const auto& anims = mesh->GetAnimations();
    if (!anims.empty()) {
        auto& animState = m_mAnimationStates[mesh];

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
    }

    if (ImGui::CollapsingHeader("Mesh Elements", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto elements = mesh->GetRenderableElements();
        for (size_t i = 0; i < elements.size(); ++i) {
            auto* element = elements[i];
            std::string label = "Element " + std::to_string(i);

            if (ImGui::TreeNode(label.c_str())) {
                dx::XMFLOAT3 ePos = element->GetPosition();
                dx::XMFLOAT3 eRot = element->GetRotation();
                dx::XMFLOAT3 eScale = element->GetScale();

                ImGui::SeparatorText("Transformation");
                if (ImGui::DragFloat3("Pos", (float*)&ePos, 0.1f)) {
                    element->SetPosition(ePos);
                }
                if (ImGui::DragFloat3("Rot", (float*)&eRot, 1.0f)) {
                    element->SetRotation(eRot);
                }
                if (ImGui::DragFloat3("Scale", (float*)&eScale, 0.05f)) {
                    element->SetScale(eScale);
                }
                ImGui::SeparatorText("Material");
                
                dx::XMFLOAT4 diffuse = element->GetDiffuse();
                if (ImGui::ColorEdit4("Diffuse", (float*)&diffuse)) {
                    element->SetDiffuse(diffuse);
                }

                dx::XMFLOAT4 specular = element->GetSpecular();
                if (ImGui::ColorEdit4("Specular", (float*)&specular)) {
                    element->SetSpecular(specular);
                }

                dx::XMFLOAT4 ambient = element->GetAmbient();
                if (ImGui::ColorEdit3("Ambient", (float*)&ambient)) {
                    element->SetAmbient(dx::XMFLOAT4(ambient.x, ambient.y, ambient.z, 1.0f));
                }

                dx::XMFLOAT4 emissive = element->GetEmissive();
                if (ImGui::ColorEdit3("Emissive", (float*)&emissive)) {
                    element->SetEmissive(dx::XMFLOAT4(emissive.x, emissive.y, emissive.z, 1.0f));
                }

                float roughness = element->GetRoughness();
                if (ImGui::SliderFloat("Roughness", &roughness, 0.0f, 1.0f)) {
                    element->SetRoughness(roughness);
                }

                float metalness = element->GetMetalness();
                if (ImGui::SliderFloat("Metalness", &metalness, 0.0f, 1.0f)) {
                    element->SetMetalness(metalness);
                }

                float specPower = element->GetSpecularPower();
                if (ImGui::SliderFloat("SpecularPower", &specPower, 1.0f, 128.0f)) {
                    element->SetSpecularPower(specPower);
                }

                float height = element->GetHeightScale();
                if (ImGui::SliderFloat("Height Scale", &height, 0.0f, 0.1f)) {
                    element->SetHeightScale(height);
                }

                ImGui::TreePop();
            }
        }
    }
}

void MainRenderer_UIComponent::DrawSimpleRenderableUI(RenderableSimpleObject* obj) {
    ImGui::Separator();
    ImGui::Text("Renderable Simple Object");

    dx::XMFLOAT3 pos = obj->GetPosition();
    dx::XMFLOAT3 rot = obj->GetRotation();
    dx::XMFLOAT3 scale = obj->GetScale();

    if (ImGui::DragFloat3("Position", (float*)&pos, 0.1f)) {
        obj->SetPosition(pos);
    }

    if (ImGui::DragFloat3("Rotation", (float*)&rot, 1.0f)) {
        obj->SetRotation(rot);
    }

    if (ImGui::DragFloat3("Scale", (float*)&scale, 0.05f)) {
        obj->SetScale(scale);
    }

    ImGui::SeparatorText("Material");

    dx::XMFLOAT4 diffuse = obj->GetDiffuse();
    if (ImGui::ColorEdit4("Diffuse", (float*)&diffuse)) {
        obj->SetDiffuse(diffuse);
    }

    dx::XMFLOAT4 specular = obj->GetSpecular();
    if (ImGui::ColorEdit4("Specular", (float*)&specular)) {
        obj->SetSpecular(specular);
    }

    dx::XMFLOAT4 ambient = obj->GetAmbient();
    if (ImGui::ColorEdit3("Ambient", (float*)&ambient)) {
        obj->SetAmbient(dx::XMFLOAT4(ambient.x, ambient.y, ambient.z, 1.0f));
    }

    dx::XMFLOAT4 emissive = obj->GetEmissive();
    if (ImGui::ColorEdit3("Emissive", (float*)&emissive)) {
        obj->SetEmissive(dx::XMFLOAT4(emissive.x, emissive.y, emissive.z, 1.0f));
    }

    float roughness = obj->GetRoughness();
    if (ImGui::SliderFloat("Roughness", &roughness, 0.0f, 1.0f)) {
        obj->SetRoughness(roughness);
    }

    float metalness = obj->GetMetalness();
    if (ImGui::SliderFloat("Metalness", &metalness, 0.0f, 1.0f)) {
        obj->SetMetalness(metalness);
    }

    float specPower = obj->GetSpecularPower();
    if (ImGui::SliderFloat("Specular Power", &specPower, 1.0f, 128.0f)) {
        obj->SetSpecularPower(specPower);
    }

    float heightScale = obj->GetHeightScale();
    if (ImGui::SliderFloat("Height Scale", &heightScale, 0.0f, 0.1f)) {
        obj->SetHeightScale(heightScale);
    }

    if (ImGui::CollapsingHeader("Textures", ImGuiTreeNodeFlags_DefaultOpen)) {
        for (int i = 0; i < std::size(m_vTextureOps); ++i) {
            ImGui::SeparatorText(m_vTextureOps[i].Name);

            ImGui::PushID(i);

            if (m_pTextureTargetObject) {
                auto tex = m_pTextureTargetObject->GetTexture(static_cast<FD3DW::TextureType>(i));
                if (tex) {
                    D3D12_GPU_DESCRIPTOR_HANDLE srv = m_pTextureTargetObject->GetTextureSRV(static_cast<FD3DW::TextureType>(i));
                    ImGui::Image((ImTextureID)srv.ptr, ImVec2(48, 48));
                }
            }

            if (ImGui::Button("Set")) {
                m_bShowTextureBrowser = true;
                m_iTextureBeingSet = i;
                m_pTextureTargetObject = obj;
            }

            if (m_pTextureTargetObject) {
                auto tex = m_pTextureTargetObject->GetTexture(static_cast<FD3DW::TextureType>(i));
                if (tex && ImGui::Button("Erase")) {
                    m_vTextureOps[i].EraseFunc(obj);
                }
            }

            ImGui::PopID();
        }
    }
}


void MainRenderer_UIComponent::DrawSkyboxUI(RenderableSkyboxObject* skybox) {
    ImGui::Separator();
    ImGui::Text("Skybox: no editable parameters");
}

void MainRenderer_UIComponent::DrawAudioUI(RenderableAudioObject* audio) {
    ImGui::Separator();
    ImGui::Text("Audio Controls");

    if (ImGui::Button("Play")) {
        audio->Play();
    }
    
    if (ImGui::Button("Stop")) {
        audio->Stop();
    }
    
    if (ImGui::Button("Restart")) {
        audio->Restart();
    }

    float volume = audio->GetVolume();
    if (ImGui::SliderFloat("Volume", &volume, 0.0f, 1.0f)) {
        audio->SetVolume(volume);
    }

    bool loop = audio->IsLoop();
    if (ImGui::Checkbox("Loop", &loop)) {
        audio->Loop(loop);
    }
}


/////////////////////////////////////////



MainRenderer_UIComponent::MainRenderer_UIComponent(MainRenderer* owner) : MainRendererComponent(owner) { 
    static const std::pair<const char*, FD3DW::TextureType> kTextureNames[] = {
        { "BASE",     FD3DW::TextureType::BASE },
        { "NORMAL",   FD3DW::TextureType::NORMAL },
        { "ROUGHNESS",FD3DW::TextureType::ROUGHNESS },
        { "METALNESS",FD3DW::TextureType::METALNESS },
        { "HEIGHT",   FD3DW::TextureType::HEIGHT },
        { "SPECULAR", FD3DW::TextureType::SPECULAR },
        { "OPACITY",  FD3DW::TextureType::OPACITY },
        { "BUMP",    FD3DW::TextureType::BUMP },
        { "EMISSIVE", FD3DW::TextureType::EMISSIVE }
    };

    for (const auto& [name, type] : kTextureNames)
    {
        m_vTextureOps.push_back({
            name,
            [this, type](RenderableSimpleObject* obj, const std::string& path) {
                obj->SetupTexture(type, path, m_pOwner->GetDevice(), m_pOwner->GetBindedCommandList());
            },
            [this, type](RenderableSimpleObject* obj) {
                obj->EraseTexture(type, m_pOwner->GetDevice());
            }
            });
    }
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


void MainRenderer_UIComponent::FileBrowser(
    bool& isOpen,
    const std::string& windowTitle,
    std::filesystem::path& currentPath,
    const std::vector<std::wstring>& supportedExtensions,
    const std::function<void(const std::filesystem::path&)>& onFileSelected,
    const std::function<std::string(const std::filesystem::path&)>& getIcon
) {
    if (!isOpen) return;

    static std::vector<std::filesystem::directory_entry> entries;
    static bool bPathChanged = true;

    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
    ImGui::Begin(windowTitle.c_str(), &isOpen, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

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

        const auto& path = entry.path();
        std::wstring ext = path.extension().wstring();
        std::string label = WStringToUTF8(path.filename().wstring());

        bool selected = false;

        if (entry.is_directory()) {
            selected = ImGui::Selectable(("Folder " + label).c_str());
            if (selected) {
                currentPath /= path.filename();
                bPathChanged = true;
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