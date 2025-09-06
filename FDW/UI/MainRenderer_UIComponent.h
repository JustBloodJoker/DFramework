#pragma once

#include <pch.h>
#include <MainRenderer/MainRendererComponent.h>
#include <UI/UIInputLayer.h>

class MainRenderer;
class TSimpleMesh;

namespace FD3DW {
	class SRV_UAVPacker;
}

class MainRenderer_UIComponent : virtual public MainRendererComponent {
public:
	MainRenderer_UIComponent()=default;
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


private:
	
	void MainWindow();

	void DrawRenderableObjectsTab();
	void DrawCameraTab();
	void DrawLightsTab();
	void DrawShadowsTab();
	void DrawEditorUtilitiesTab();

	void SceneBrowser();
	void AudioBrowser();
	void SkyboxBrowser();
	void TextureBrowser();
	void SaveSceneBrowser();
	void LoadSceneBrowser();

private:
	void SceneFileBrowser(bool& isOpen, const std::string& title, bool isSave, const std::function<void(const std::filesystem::path&)>& onConfirm);
	void FileBrowser(bool& isOpen, const std::string& windowTitle, std::filesystem::path& currentPath, const std::vector<std::wstring>& supportedExtensions, const std::function<void(const std::filesystem::path&)>& onFileSelected, const std::function<std::string(const std::filesystem::path&)>& getIcon = nullptr);
	std::vector<std::filesystem::directory_entry> GetDirectoryEntries(const std::filesystem::path& dir);
	std::string WStringToUTF8(const std::wstring& wstr);

private:
	std::unique_ptr<UIInputLayer> m_pUILayer = nullptr;
	std::unique_ptr<FD3DW::SRV_UAVPacker> m_pUISRVPack;
	bool m_bIsInited = false;

public:

	void ProcessAfterRenderUICalls();

protected:

	void AddCallToPull(std::function<void(void)> foo);
	std::vector< std::function<void(void)>> m_vCallsAfterRender;

private:
	//UI HELP FIELDS
	struct TextureOpEntry {
		const char* Name;
		std::function<void(TSimpleMesh* obj, const std::string& path)> SetFunc;
		std::function<void(TSimpleMesh* obj)> EraseFunc;
	};

	struct AnimationUIState {
		int SelectedAnimIndex = 0;
		bool IsFreeze = false;
	};

	bool m_bShowSceneBrowser = false;
	bool m_bShowAudioBrowser = false;
	bool m_bShowSkyboxBrowser = false;
	bool m_bShowTextureBrowser = false;
	std::filesystem::path m_xCurrentPath = SCENES_DEFAULT_PATH;
	int m_iSelectedLightIndex = 0;
	std::vector<TextureOpEntry> m_vTextureOps;
	int m_iTextureBeingSet = -1;
	int m_iSelectedObjectIndex = -1;
	bool m_bShowSaveSceneBrowser = false;
	bool m_bShowLoadSceneBrowser = false;
	std::vector<std::filesystem::directory_entry> m_vEntries;
	bool m_bIsPathChanged = true;

};
