#include <UI/MainRenderer_UIComponent.h>
#include <UI/UIInputLayer.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_dx12.h>
#include <backends/imgui_impl_win32.h>
#include <MainRenderer/MainRenderer.h>
#include <D3DFramework/GraphicUtilites/ResourcePacker.h>
#include <World/World.h>



static const std::vector<std::wstring> s_vSupportedSceneExts = { L".gltf", L".fbx" };
static const std::vector<std::wstring> s_vSupportedSkyboxExts = { L".hdr", L".dds", L".skybox" };
static const std::vector<std::wstring> s_vSupportedAudioExts = { L".wav" };
static const std::vector<std::wstring> s_vSupportedTextureExts = { L".jpg", L".png" };
static const std::vector<std::wstring> s_vSupportedEngineFileExts = { L".fdw" };

void MainRenderer_UIComponent::DrawUI() {
    MainWindow();
    LoadWorldBrowser();
    SaveWorldBrowser();
    SkyboxBrowser();
    SceneBrowser();
    TextureBrowser();
}

void MainRenderer_UIComponent::MainWindow() {
    ImGui::Begin("ECS Editor");

    if (ImGui::BeginTabBar("MainTabs")) {

        if (ImGui::BeginTabItem("World")) {
            DrawWorldTab();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Editor")) {
            DrawEditorTab();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

void MainRenderer_UIComponent::DrawWorldTab() {
    if (ImGui::BeginTabBar("WorldTabs")) {

        if (ImGui::BeginTabItem("Entities")) {
            DrawWorld_Entities();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Add Entity")) {
            DrawWorld_AddEntity();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}
void MainRenderer_UIComponent::DrawWorld_Entities() {
    auto world = m_pOwner->GetWorld();
    const auto& entities = world->GetEntities();

    ImGui::SeparatorText("Entities");

    static char renameBuffer[256] = {};
    static void* renamingPtr = nullptr;
    static bool requestFocus = false;

    for (size_t i = 0; i < entities.size(); ++i) {
        auto* entity = entities[i];
        std::string entityName = entity->GetName();
        std::string entityLabelID = "##" + std::to_string(reinterpret_cast<uintptr_t>(entity));
        bool entityOpen = false;

        if (renamingPtr == entity) {
            ImGui::PushID(entity);
            ImGui::SetNextItemWidth(200.0f);
            
            if (requestFocus) {
                ImGui::SetKeyboardFocusHere();
                requestFocus = false;
            }

            if (ImGui::InputText("##RenameEntity", renameBuffer, sizeof(renameBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
                entity->SetName(renameBuffer);
                renamingPtr = nullptr;
            }
            if (ImGui::IsItemDeactivated()) {
                renamingPtr = nullptr;
            }
            entityOpen = ImGui::TreeNodeEx((entityLabelID + "_dummy").c_str(), ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_NoAutoOpenOnLog, "");
            ImGui::PopID();
        }
        else {
            entityOpen = ImGui::TreeNode(entityLabelID.c_str(), "%s", entityName.c_str());
        }

        if (ImGui::BeginPopupContextItem(entityLabelID.c_str())) {
            if (ImGui::MenuItem("Delete")) {
                AddCallToPull([world, entity]() {
                    world->DestroyEntity(entity);
                    });
            }
            if (ImGui::MenuItem("Rename")) {
                strncpy_s(renameBuffer, entity->GetName().c_str(), sizeof(renameBuffer));
                renameBuffer[sizeof(renameBuffer) - 1] = '\0';
                renamingPtr = entity;
                requestFocus = true;
            }
            ImGui::EndPopup();
        }

        if (entityOpen) {
            DrawEntityInspector(entity);

            if (ImGui::TreeNode(("Components##" + std::to_string(reinterpret_cast<uintptr_t>(entity))).c_str(),
                "Components")) {
                auto components = entity->GetComponents<IComponent>();

                for (size_t j = 0; j < components.size(); ++j) {
                    auto* comp = components[j];
                    std::string compName = comp->GetName();
                    std::string compLabelID = "##" + std::to_string(reinterpret_cast<uintptr_t>(comp));
                    bool compOpen = false;

                    if (renamingPtr == comp) {
                        ImGui::PushID(comp);
                        ImGui::SetNextItemWidth(200.0f);

                        if (requestFocus) {
                            ImGui::SetKeyboardFocusHere();
                            requestFocus = false;
                        }

                        if (ImGui::InputText("##RenameComp", renameBuffer, sizeof(renameBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
                            comp->SetName(renameBuffer);
                            renamingPtr = nullptr;
                        }
                        if (ImGui::IsItemDeactivated()) {
                            renamingPtr = nullptr;
                        }
                        compOpen = ImGui::TreeNodeEx((compLabelID + "_dummy").c_str(), ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_NoAutoOpenOnLog, "");
                        ImGui::PopID();
                    }
                    else {
                        compOpen = ImGui::TreeNode(compLabelID.c_str(), "%s", compName.c_str());
                    }

                    if (ImGui::BeginPopupContextItem(compLabelID.c_str())) {
                        if (ImGui::MenuItem("Delete")) {
                            AddCallToPull([entity, comp]() {
                                entity->RemoveComponent(comp);
                                });
                        }
                        if (ImGui::MenuItem("Rename")) {
                            strncpy_s(renameBuffer, comp->GetName().c_str(), sizeof(renameBuffer));
                            renameBuffer[sizeof(renameBuffer) - 1] = '\0';
                            renamingPtr = comp;
                            requestFocus = true;
                        }
                        ImGui::EndPopup();
                    }

                    if (compOpen) {
                        DrawComponentInspector(comp);
                        ImGui::TreePop();
                    }
                }
                ImGui::TreePop();
            }
            ImGui::TreePop();
        }
    }
}


void MainRenderer_UIComponent::DrawEntityInspector(ComponentHolder* entity) {
    bool active = entity->IsActive();
    if (ImGui::Checkbox("Active", &active)) {
        entity->Activate(active);
    }
    
    if (auto light = dynamic_cast<TLight*>(entity)) {
        DrawLightInspector(light);
    }
    else if (auto mesh = dynamic_cast<TMesh*>(entity)) {
        DrawMeshInspector(mesh);
    }
    else if (auto baseCamera = dynamic_cast<TBaseCamera*>(entity)) {
        DrawBaseCameraInspector(baseCamera);
    }
}

void MainRenderer_UIComponent::DrawMeshInspector(TMesh* mesh) {
    auto pos = mesh->GetPosition();
    if (EditFloat3("Position", pos, 20.0f)) {
        mesh->SetPosition(pos);
    }
    auto rot = mesh->GetRotation();
    if (EditFloat3("Rotation", rot)) {
        mesh->SetRotation(rot);
    } 
    auto scale = mesh->GetScale();
    if (EditFloat3("Scaling", scale)) {
        mesh->SetScale(scale);
    }

    if (auto scene = dynamic_cast<TScene*>(mesh)) {
        DrawSceneInspector(scene);
    }
    else if (auto simpleMesh = dynamic_cast<TSimpleMesh*>(mesh)) {
        DrawSimpleMeshInspector(simpleMesh);
    }
}

void MainRenderer_UIComponent::DrawSceneInspector(TScene* mesh) {
    if (mesh->IsHaveAnimation()) {
        ImGui::SeparatorText("Animations");

        auto animations = mesh->GetAnimations();
        static int currentAnim = -1;

        if (ImGui::BeginCombo("Animation", (currentAnim >= 0 && currentAnim < (int)animations.size()) ? animations[currentAnim].c_str() : "None"))
        {
            for (int i = 0; i < animations.size(); i++) {
                bool isSelected = (currentAnim == i);
                if (ImGui::Selectable(animations[i].c_str(), isSelected)) {
                    currentAnim = i;
                    mesh->Play(animations[i]);
                }
                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        if (mesh->IsPlaying()) 
        {
            if (ImGui::Button("Stop")) {
                mesh->Stop();
            }
        }
        else 
        {
            if (currentAnim >= 0 && ImGui::Button("Play")) {
                mesh->Play(animations[currentAnim]);
            }
        }

        bool freeze = mesh->IsFreeze();
        if (ImGui::Checkbox("Freeze", &freeze)) {
            mesh->Freeze(freeze);
        }
    }
}

void MainRenderer_UIComponent::DrawSimpleMeshInspector(TSimpleMesh* mesh) {
    ImGui::SeparatorText("Material Properties");

    dx::XMFLOAT4 diffuse = mesh->GetDiffuse();
    if (EditColor4("Diffuse", diffuse)) {
        mesh->SetDiffuse(diffuse);
    }
    dx::XMFLOAT4 ambient = mesh->GetAmbient();
    if (EditColor4("Ambient", ambient)) {
        mesh->SetAmbient(ambient);
    }
    dx::XMFLOAT4 specular = mesh->GetSpecular();
    if (EditColor4("Specular", specular)) {
        mesh->SetSpecular(specular);
    }
    dx::XMFLOAT4 emissive = mesh->GetEmissive();
    if (EditColor4("Emissive", emissive)) {
        mesh->SetEmissive(emissive);
    }

    float roughness = mesh->GetRoughness();
    float metalness = mesh->GetMetalness();
    float specularPower = mesh->GetSpecularPower();
    float heightScale = mesh->GetHeightScale();

    if (EditFloat("Roughness", roughness, 0.01f)) mesh->SetRoughness(roughness);
    if (EditFloat("Metalness", metalness, 0.01f)) mesh->SetMetalness(metalness);
    if (EditFloat("SpecularPower", specularPower, 0.1f)) mesh->SetSpecularPower(specularPower);
    if (EditFloat("HeightScale", heightScale, 0.01f)) mesh->SetHeightScale(heightScale);

    ImGui::SeparatorText("Textures");

    static const char* s_textureLabels[TEXTURE_TYPE_SIZE] = {
        "Base Color", "Normal", "Roughness", "Metalness",
        "Height", "Specular", "Opacity", "Ambient", "Emissive"
    };

    for (int i = 0; i < TEXTURE_TYPE_SIZE; i++) {
        FD3DW::TextureType type = static_cast<FD3DW::TextureType>(i);
        const char* label = s_textureLabels[i];

        ImGui::Separator();
        ImGui::Text("%s", label);

        if (mesh->GetTexture(type)!=nullptr ) {
            auto srvHandle = mesh->GetTextureSRV(type);

            std::string imgId = std::string(label) + "##" + std::to_string(i);
            if (ImGui::ImageButton(imgId.c_str(), (ImTextureID)srvHandle.ptr, ImVec2(64, 64))) {
                m_bShowTextureBrowser = true;
                m_onTextureSelected = [this, mesh, type](const std::filesystem::path& selectedPath) {
                    mesh->SetupTexture(type, selectedPath.string(), m_pOwner->GetDevice(), m_pCurrentUIList);
                };
            }
        }
        else {
            ImGui::Text("<empty>");
            if (ImGui::Button(std::string("Set ").append(label).c_str())) {
                m_bShowTextureBrowser = true;
                m_onTextureSelected = [this, mesh, type](const std::filesystem::path& selectedPath) {
                    mesh->SetupTexture(type, selectedPath.string(), m_pOwner->GetDevice(), m_pCurrentUIList);
                };
            }
        }

        ImGui::SameLine();
        if (ImGui::Button(std::string("Clear ").append(label).c_str())) {
            mesh->EraseTexture(type, m_pOwner->GetDevice());
        }
    }
}

void MainRenderer_UIComponent::DrawBaseCameraInspector(TBaseCamera* baseCamera) {
    
    float speed = baseCamera->GetCameraSpeed();
    if (ImGui::DragFloat("Camera Speed", &speed, 1.f, 0.01f, 1000.0f)) {
        baseCamera->SetCameraSpeed(speed);
    }

    if (ImGui::Button("Reset Position")) {
        baseCamera->ResetPosition();
    }

    if (auto defaultCamera = dynamic_cast<TDefaultCamera*>(baseCamera)) {
        DrawDefaultCameraInspector(defaultCamera);
    }
}

void MainRenderer_UIComponent::DrawDefaultCameraInspector(TDefaultCamera* baseCamera) {
    
}

void MainRenderer_UIComponent::DrawLightInspector(TLight* entity) {

    float intensity = entity->GetLightIntensity();
    if (EditFloat("Intensity", intensity)) {
        entity->SetLightIntensity(intensity);
    }

    dx::XMFLOAT3 color = entity->GetLightColor();
    if (EditColor3("Color", color)) {
        entity->SetLightColor(color);
    }


    if (auto directionalLight = dynamic_cast<TDirectionalLight*>(entity)) {
        DrawDirectionalLightInspector(directionalLight);
    } else  if (auto pointLight = dynamic_cast<TPointLight*>(entity)) {
        DrawPointLightInspector(pointLight);
    }
    else  if (auto spotLight = dynamic_cast<TSpotLight*>(entity)) {
        DrawSpotLightInspector(spotLight);
    }
    else  if (auto rectLight = dynamic_cast<TRectLight*>(entity)) {
        DrawRectLightInspector(rectLight);
    }
}

void MainRenderer_UIComponent::DrawDirectionalLightInspector(TDirectionalLight* entity) {
    dx::XMFLOAT3 dir = entity->GetLightDirection();
    if (EditFloat3("Direction", dir)) {
        entity->SetLightDirection(dir);
    }
}

void MainRenderer_UIComponent::DrawPointLightInspector(TPointLight* entity) {
    dx::XMFLOAT3 pos = entity->GetLightPosition();
    if (EditFloat3("Position", pos, 20.0f)) {
        entity->SetLightPosition(pos);
    }

    float srcRadius = entity->GetLightSourceRadius();
    if (EditFloat("Source Radius", srcRadius)) {
        entity->SetLightSourceRadius(srcRadius);
    }

    float attenRadius = entity->GetLightAttenuationRadius();
    if (EditFloat("Attenuation Radius", attenRadius)) {
        entity->SetLightAttenuationRadius(attenRadius);
    }
}

void MainRenderer_UIComponent::DrawSpotLightInspector(TSpotLight* entity) {
    dx::XMFLOAT3 pos = entity->GetLightPosition();
    if (EditFloat3("Position", pos, 20.0f)) {
        entity->SetLightPosition(pos);
    }

    dx::XMFLOAT3 dir = entity->GetLightDirection();
    if (EditFloat3("Direction", dir)) {
        entity->SetLightDirection(dir);
    }

    float outer = entity->GetLightOuterConeAngle();
    if (EditFloat("Outer Cone", outer)) {
        entity->SetLightOuterConeAngle(outer);
    }

    float inner = entity->GetLightInnerConeAngle();
    if (EditFloat("Inner Cone", inner)) {
        entity->SetLightInnerConeAngle(inner);
    }

    float atten = entity->GetLightAttenuationRadius();
    if (EditFloat("Attenuation Radius", atten)) {
        entity->SetLightAttenuationRadius(atten);
    }
}

void MainRenderer_UIComponent::DrawRectLightInspector(TRectLight* entity) {
    dx::XMFLOAT3 pos = entity->GetLightPosition();
    if (EditFloat3("Position", pos, 20.0f)) {
        entity->SetLightPosition(pos);
    }

    dx::XMFLOAT3 rot = entity->GetLightRotation();
    if (EditFloat3("Rotation", rot)) {
        entity->SetLightRotation(rot);
    }

    dx::XMFLOAT2 size = entity->GetLightRectSize();
    if (EditFloat2("Rect Size", size)) {
        entity->SetLightRectSize(size);
    }
}

void MainRenderer_UIComponent::DrawComponentInspector(IComponent* comp) {
    bool active = comp->IsActive();
    if (ImGui::Checkbox("Active", &active)) {
        comp->Activate(active);
    }

    ImGui::Separator();

    if (auto light = dynamic_cast<LightComponent*>(comp)) {
        DrawLightComponentInspector(light);
    }
    else if (auto mesh = dynamic_cast<MeshComponent*>(comp)) {
        DrawMeshComponentInspector(mesh);
    }
    else if (auto animation = dynamic_cast<AnimationComponent*>(comp)) {
        DrawAnimationComponentInspector(animation);
    }
}

void MainRenderer_UIComponent::DrawLightComponentInspector(LightComponent* entity) {
    LightComponentData data = entity->GetLightComponentData();
    bool changed = false;

    const char* typeNames[] = { "Point", "Spot", "Directional", "Rect" };
    int currentType = data.LightType;
    if (ImGui::Combo("Light Type", &currentType, typeNames, IM_ARRAYSIZE(typeNames))) {
        data.LightType = currentType;
        changed |= true;
    }

    ImGui::Separator();

    changed |= EditColor3("Color", data.Color);
    changed |= EditFloat("Intensity", data.Intensity, 0.1f);

    ImGui::Separator();

    switch (static_cast<NLightComponentTypes>(data.LightType)) {
        case NLightComponentTypes::PointLight: {
            changed |= EditFloat3("Position", data.Position);
            changed |= EditFloat("Attenuation Radius", data.AttenuationRadius);
            changed |= EditFloat("Source Radius", data.SourceRadius);
            break;
        }
        case NLightComponentTypes::SpotLight: {
            changed |= EditFloat3("Position", data.Position);
            changed |= EditFloat3("Direction", data.Direction);
            changed |= EditFloat("Attenuation Radius", data.AttenuationRadius);
            changed |= EditFloat("Inner Cone Angle", data.InnerConeAngle);
            changed |= EditFloat("Outer Cone Angle", data.OuterConeAngle);
            break;
        }
        case NLightComponentTypes::DirectionalLight: {
            changed |= EditFloat3("Direction", data.Direction);
            break;
        }
        case NLightComponentTypes::RectLight: {
            changed |= EditFloat3("Position", data.Position);
            changed |= EditFloat3("Rotation", data.Rotation);
            changed |= EditFloat2("Rect Size", data.RectSize);
            break;
        }
    }

    if(changed) entity->SetLightComponentData(data);
}

void MainRenderer_UIComponent::DrawMeshComponentInspector(MeshComponent* meshComponent) {
    auto pos = meshComponent->GetPosition();
    if (EditFloat3("Position", pos, 20.0f)) {
        meshComponent->SetPosition(pos);
    }
    auto rot = meshComponent->GetRotation();
    if (EditFloat3("Rotation", rot)) {
        meshComponent->SetRotation(rot);
    }
    auto scale = meshComponent->GetScale();
    if (EditFloat3("Scaling", scale)) {
        meshComponent->SetScale(scale);
    }

    bool ignoreParentPos = meshComponent->IsIgnoreParentPosition();
    bool ignoreParentRot = meshComponent->IsIgnoreParentRotation();
    bool ignoreParentScl = meshComponent->IsIgnoreParentScaling();

    if (ImGui::Checkbox("Ignore Parent Position", &ignoreParentPos)) {
        meshComponent->IgnoreParentPosition(ignoreParentPos);
    }
    if (ImGui::Checkbox("Ignore Parent Rotation", &ignoreParentRot)) {
        meshComponent->IgnoreParentRotation(ignoreParentRot);
    }
    if (ImGui::Checkbox("Ignore Parent Scaling", &ignoreParentScl)) {
        meshComponent->IgnoreParentScaling(ignoreParentScl);
    }
}

void MainRenderer_UIComponent::DrawAnimationComponentInspector(AnimationComponent* animComponent) {
    auto animations = animComponent->GetAnimations();
    static int currentAnim = -1;

    if (ImGui::BeginCombo("Animation",
        (currentAnim >= 0 && currentAnim < (int)animations.size())
        ? animations[currentAnim].c_str() : "None"))
    {
        for (int i = 0; i < animations.size(); i++) {
            bool isSelected = (currentAnim == i);
            if (ImGui::Selectable(animations[i].c_str(), isSelected)) {
                currentAnim = i;
                animComponent->Play(animations[i]);
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    if (animComponent->IsPlaying()) {
        if (ImGui::Button("Stop")) {
            animComponent->Stop();
        }
    }
    else {
        if (currentAnim >= 0 && ImGui::Button("Play")) {
            animComponent->Play(animations[currentAnim]);
        }
    }

    bool freeze = animComponent->IsFreeze();
    if (ImGui::Checkbox("Freeze", &freeze)) {
        animComponent->Freeze(freeze);
    }
}



void MainRenderer_UIComponent::DrawWorld_AddEntity() {
    auto world = m_pOwner->GetWorld();
    ImGui::SeparatorText("Add Entity");

    if (ImGui::Button("Empty Entity")) {
        AddCallToPull([world]() { world->CreateEmptyEntity(); });
    }

    if (ImGui::Button("Default Camera")) {
        AddCallToPull([world]() { world->CreateDefaultCamera(); });
    }

    if (ImGui::Button("Directional Light")) {
        AddCallToPull([world]() { world->CreateDirectionalLight(); });
    }

    if (ImGui::Button("Point Light")) {
        AddCallToPull([world]() { world->CreatePointLight(); });
    }

    if (ImGui::Button("Spot Light")) {
        AddCallToPull([world]() { world->CreateSpotLight(); });
    }

    if (ImGui::Button("Rect Light")) {
        AddCallToPull([world]() { world->CreateRectLight(); });
    }

    if (ImGui::Button("Simple Plane")) {
        AddCallToPull([world]() { world->CreateSimplePlane(); });
    }

    if (ImGui::Button("Simple Cube")) {
        AddCallToPull([world]() { world->CreateSimpleCube(); });
    }

    if (ImGui::Button("Simple Cone")) {
        AddCallToPull([world]() { world->CreateSimpleCone(); });
    }

    if (ImGui::Button("Simple Sphere")) {
        AddCallToPull([world]() { world->CreateSimpleSphere(); });
    }

    ImGui::Separator();

    if (ImGui::Button("Skybox...")) {
        m_bShowSkyboxBrowser = true;
    }

    if (ImGui::Button("Scene...")) {
        m_bShowSceneBrowser = true;
    }
}

void MainRenderer_UIComponent::DrawEditorTab() {
    if (ImGui::BeginTabBar("EditorTabs")) {

        if (ImGui::BeginTabItem("Packaging")) {
            DrawEditor_Packaging();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Systems")) {
            DrawEditor_Systems();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}

void MainRenderer_UIComponent::DrawEditor_Packaging() {
    ImGui::SeparatorText("Scene I/O");

    if (ImGui::Button("Save Scene")) {
        m_bShowSaveWorldBrowser = true;
    }
    if (ImGui::Button("Load Scene")) {
        m_bShowLoadWorldBrowser = true;
    }
}

void MainRenderer_UIComponent::DrawEditor_Systems() {
    ImGui::SeparatorText("Systems");
    
    if (ImGui::TreeNodeEx("Global Render System", ImGuiTreeNodeFlags_DefaultOpen)) {
        DrawEditor_GlobalRenderSystem();
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("Shadow System", ImGuiTreeNodeFlags_DefaultOpen)) {
        DrawEditor_ShadowSystem();
        ImGui::TreePop();
    }
}

void MainRenderer_UIComponent::DrawEditor_GlobalRenderSystem() {
    bool preDepth = m_pOwner->IsEnabledPreDepth();
    if (ImGui::Checkbox("Enable PreDepth", &preDepth)) {
        m_pOwner->EnablePreDepth(preDepth);
    }

    MeshCullingType cullingType = m_pOwner->GetMeshCullingType();
    int type = static_cast<int>(cullingType);

    const char* items[] = { "None", "GPU" };
    if (ImGui::Combo("Mesh Culling", &type, items, IM_ARRAYSIZE(items))) {
        m_pOwner->SetMeshCullingType(static_cast<MeshCullingType>(type));
    }
}

void MainRenderer_UIComponent::DrawEditor_ShadowSystem() {
    auto enabled = m_pOwner->IsShadowEnabled();

    if (!enabled) return;

    RTShadowSystemConfig cfg = m_pOwner->GetRTShadowConfig();

    bool changed = false;
    ImGui::SeparatorText("Temporal Reprojection");
    changed |= ImGui::DragFloat("Temporal Feedback Min", &cfg.TemporalFeedbackMin, 0.01f, 0.0f, 1.0f);
    changed |= ImGui::DragFloat("Temporal Feedback Max", &cfg.TemporalFeedbackMax, 0.01f, 0.0f, 1.0f);
    changed |= ImGui::DragFloat("Reproj Dist Threshold", &cfg.ReprojDistThreshold, 0.001f, 0.0f, 1.0f);
    changed |= ImGui::DragFloat("Normal Threshold", &cfg.NormalThreshold, 0.01f, 0.0f, 1.0f);

    ImGui::SeparatorText("Bilateral Filter");
    changed |= ImGui::DragFloat("Sigma S", &cfg.SigmaS, 0.1f, 0.0f, 10.0f);
    changed |= ImGui::DragFloat("Sigma R", &cfg.SigmaR, 0.01f, 0.0f, 1.0f);
    changed |= ImGui::DragInt("Kernel Radius", &cfg.KernelRadius, 1, 1, 15);

    if(changed) m_pOwner->SetRTShadowConfig(cfg);
}


void MainRenderer_UIComponent::TextureBrowser() {
    FileBrowser(
        m_bShowTextureBrowser,
        "Texture Browser",
        m_xCurrentPath,
        s_vSupportedTextureExts,
        [this](const std::filesystem::path& path) {
            if (m_onTextureSelected) {
                m_onTextureSelected(path);
                m_onTextureSelected = nullptr;
            }
        },
        [](const std::filesystem::path& path) {
            return "Texture ";
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
            AddCallToPull([this, path]() {
                m_pOwner->GetWorld()->CreateSkybox(path.string());
                });
        },
        [](const std::filesystem::path& path) {
            return "Skybox ";
        }
    );
}

void MainRenderer_UIComponent::SceneBrowser() {
    FileBrowser(
        m_bShowSceneBrowser,
        "Scene Browser",
        m_xCurrentPath,
        s_vSupportedSceneExts,
        [this](const std::filesystem::path& path) {
            AddCallToPull([this, path]() {
                m_pOwner->GetWorld()->CreateScene(path.string());
                });
        },
        [](const std::filesystem::path& path) {
            return "Scene ";
        }
    );
}

void MainRenderer_UIComponent::SaveWorldBrowser() {
    SceneFileBrowser(m_bShowSaveWorldBrowser, "Save Scene", true,
        [this](const std::filesystem::path& path) {
            AddCallToPull([this, path]() { m_pOwner->SaveActiveWorld(path.string()); });
        });
}

void MainRenderer_UIComponent::LoadWorldBrowser() {
    SceneFileBrowser(m_bShowLoadWorldBrowser, "Load Scene", false,
        [this](const std::filesystem::path& path) {
            AddCallToPull([this, path]() { m_pOwner->LoadWorld(path.string()); });
        });
}

void MainRenderer_UIComponent::SceneFileBrowser(bool& isOpen, const std::string& title, bool isSave,
    const std::function<void(const std::filesystem::path&)>& onConfirm)
{
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
                if (fullPath.extension().empty()) fullPath += ".fdw";
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
    const std::function<std::string(const std::filesystem::path&)>& getIcon)
{
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
            if (!supportedExtensions.empty()
                && std::find(supportedExtensions.begin(), supportedExtensions.end(), ext) == supportedExtensions.end())
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

void MainRenderer_UIComponent::InitImGui() {
    if (m_bIsInited) return;

    m_pUISRVPack = m_pOwner->CreateSRVPack(1u);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplWin32_Init(m_pOwner->GETHWND());

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

    m_pCurrentUIList = list;
    DrawUI();

    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_pCurrentUIList);
    m_pCurrentUIList = nullptr;
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
    if (foo) m_vCallsAfterRender.push_back(std::move(foo));
}

void MainRenderer_UIComponent::ProcessAfterRenderUICalls() {
    auto vv = std::move(m_vCallsAfterRender);
    m_vCallsAfterRender.clear();
    for (auto& foo : vv) if (foo) foo();
}

void MainRenderer_UIComponent::AfterConstruction() {
    InitImGui();
    m_pUILayer = std::make_unique<UIInputLayer>(this);
    m_pUILayer->AddToRouter(m_pOwner->GetInputRouter());
}

void MainRenderer_UIComponent::BeforeDestruction() {
    ShutDownImGui();
    m_pUILayer->AddToRouter(nullptr);
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
    if (size_needed <= 0 || size_needed > 1024 * 1024) return "<invalid path>";
    std::string result(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), &result[0], size_needed, nullptr, nullptr);
    return result;
}


bool MainRenderer_UIComponent::EditFloat(const char* label, float& value, float step) {
    return ImGui::DragFloat(label, &value, step, 0.0f, 0.0f, "%.3f");
}

bool MainRenderer_UIComponent::EditFloat3(const char* label, dx::XMFLOAT3& vec, float step ) {
    float tmp[3] = { vec.x, vec.y, vec.z };
    if (ImGui::DragFloat3(label, tmp, step)) {
        vec = { tmp[0], tmp[1], tmp[2] };
        return true;
    }
    return false;
}

bool MainRenderer_UIComponent::EditFloat2(const char* label, dx::XMFLOAT2& vec, float step) {
    float tmp[2] = { vec.x, vec.y };
    if (ImGui::DragFloat2(label, tmp, step)) {
        vec = { tmp[0], tmp[1] };
        return true;
    }
    return false;
}

bool MainRenderer_UIComponent::EditColor4(const char* label, dx::XMFLOAT4& col) {
    float tmp[4] = { col.x, col.y, col.z, col.w };
    if (ImGui::ColorEdit4(label, tmp)) {
        col = { tmp[0], tmp[1], tmp[2], tmp[3] };
        return true;
    }
    return false;
}

bool MainRenderer_UIComponent::EditColor3(const char* label, dx::XMFLOAT3& col) {
    float tmp[3] = { col.x, col.y, col.z };
    if (ImGui::ColorEdit3(label, tmp)) {
        col = { tmp[0], tmp[1], tmp[2] };
        return true;
    }
    return false;
}