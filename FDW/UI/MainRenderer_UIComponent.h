#pragma once

#include <pch.h>
#include <MainRenderer/MainRendererComponent.h>
#include <UI/UIInputLayer.h>

class MainRenderer;

namespace FD3DW { class SRV_UAVPacker; }

class IComponent;
class LightComponent;
class MeshComponent;
class AnimationComponent;
class AudioComponent;

class ComponentHolder;
class TLight;
class TDirectionalLight;
class TPointLight;
class TSpotLight;
class TRectLight;
class TMesh;
class TScene;
class TSimpleMesh;
class TBaseCamera;
class TDefaultCamera;
class TAudio;



class MainRenderer_UIComponent : virtual public MainRendererComponent {
public:
    MainRenderer_UIComponent() = default;
    virtual ~MainRenderer_UIComponent() = default;

public:
    void InitImGui();
    void RenderImGui(ID3D12GraphicsCommandList* list);
    void ShutDownImGui();
    bool ImGuiInputProcess(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

public:
    virtual void AfterConstruction() override;
    virtual void BeforeDestruction() override;

private:
    void DrawUI();
    void MainWindow();

    void DrawWorldTab();
    void DrawWorld_Entities();


    void DrawEntityInspector(ComponentHolder* entity);
    void DrawMeshInspector(TMesh* mesh);
    void DrawSceneInspector(TScene* mesh);
    void DrawSimpleMeshInspector(TSimpleMesh* mesh);

    void DrawBaseCameraInspector(TBaseCamera* baseCamera);
    void DrawDefaultCameraInspector(TDefaultCamera* baseCamera);

    void DrawLightInspector(TLight* entity);
    void DrawDirectionalLightInspector(TDirectionalLight* entity);
    void DrawPointLightInspector(TPointLight* entity);
    void DrawSpotLightInspector(TSpotLight* entity);
    void DrawRectLightInspector(TRectLight* entity);
    
    void DrawAudioInspector(TAudio* audio);

    void DrawComponentInspector(IComponent* comp);
    void DrawLightComponentInspector(LightComponent* entity);

    void DrawMeshComponentInspector(MeshComponent* meshComponent);
    void DrawAnimationComponentInspector(AnimationComponent* animComponent);

    void DrawAudioComponentInspector(AudioComponent* audio);

    void DrawWorld_AddEntity();

    void DrawEditorTab();
    void DrawEditor_Packaging();

    void DrawEditor_Systems();
    void DrawEditor_GlobalRenderSystem();
    void DrawEditor_ShadowSystem();


    void TextureBrowser();
    void SkyboxBrowser();
    void SceneBrowser();
    void AudioBrowser();
    void SaveWorldBrowser();
    void LoadWorldBrowser();


    void SceneFileBrowser(bool& isOpen, const std::string& title, bool isSave,
        const std::function<void(const std::filesystem::path&)>& onConfirm);
    void FileBrowser(bool& isOpen, const std::string& windowTitle, std::filesystem::path& currentPath,
        const std::vector<std::wstring>& supportedExtensions,
        const std::function<void(const std::filesystem::path&)>& onFileSelected,
        const std::function<std::string(const std::filesystem::path&)>& getIcon = nullptr);
    std::vector<std::filesystem::directory_entry> GetDirectoryEntries(const std::filesystem::path& dir);
    std::string WStringToUTF8(const std::wstring& wstr);


    bool EditColor4(const char* label, dx::XMFLOAT4& col);
    bool EditColor3(const char* label, dx::XMFLOAT3& col);
    bool EditFloat2(const char* label, dx::XMFLOAT2& vec, float step = 0.1f);
    bool EditFloat3(const char* label, dx::XMFLOAT3& vec, float step = 0.1f);
    bool EditFloat(const char* label, float& value, float step = 0.1f);

public:
    void ProcessAfterRenderUICalls();

protected:
    void AddCallToPull(std::function<void(void)> foo);
    std::vector<std::function<void(void)>> m_vCallsAfterRender;

private:
    ID3D12GraphicsCommandList* m_pCurrentUIList = nullptr;
    std::unique_ptr<UIInputLayer> m_pUILayer = nullptr;
    std::unique_ptr<FD3DW::SRV_UAVPacker> m_pUISRVPack;
    bool m_bIsInited = false;

    struct EntityUIState {
        bool showComponents = false;
    };
    std::unordered_map<ComponentHolder*, EntityUIState> m_EntityUi;

    bool m_bShowSkyboxBrowser = false;
    bool m_bShowSceneBrowser = false;
    bool m_bShowAudioBrowser = false;

    bool m_bShowTextureBrowser = false;
    std::function<void(const std::filesystem::path&)> m_onTextureSelected;

    bool m_bShowSaveWorldBrowser = false;
    bool m_bShowLoadWorldBrowser = false;
    std::filesystem::path m_xCurrentPath = SCENES_DEFAULT_PATH;
    std::vector<std::filesystem::directory_entry> m_vEntries;
    bool m_bIsPathChanged = true;

    int m_iSelectedEntityIndex = -1;
};

// ------------------------------------------------------
// реализаци€ Ч см. MainRenderer_UIComponent.cpp
