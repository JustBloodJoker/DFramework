#pragma once

#include <pch.h>
#include <MainRenderer/MainRendererComponent.h>
#include <UI/UIInputLayer.h>

class MainRenderer;
class BaseRenderableObject;
class RenderableSimpleObject;
class RenderableSkyboxObject;
class RenderableAudioObject;
class RenderableMesh;

namespace FD3DW {
	class SRVPacker;
}

class MainRenderer_UIComponent : virtual public MainRendererComponent {
public:
	MainRenderer_UIComponent(MainRenderer* owner);
	virtual ~MainRenderer_UIComponent() = default;

public:
	void InitImGui();
	void RenderImGui();
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

	void SceneBrowser();
	void AudioBrowser();
	void SkyboxBrowser();
	void TextureBrowser();


	void ElementParamSetter();
	
	void DrawMeshUI(RenderableMesh* mesh);
	void DrawSimpleRenderableUI(RenderableSimpleObject* obj);
	void DrawSkyboxUI(RenderableSkyboxObject* skybox);
	void DrawAudioUI(RenderableAudioObject* audio);

private:
	void FileBrowser(bool& isOpen, const std::string& windowTitle, std::filesystem::path& currentPath, const std::vector<std::wstring>& supportedExtensions, const std::function<void(const std::filesystem::path&)>& onFileSelected, const std::function<std::string(const std::filesystem::path&)>& getIcon = nullptr);
	std::vector<std::filesystem::path> FindSceneFiles(const std::wstring& root, const std::vector<std::wstring>& extensions);
	std::vector<std::filesystem::directory_entry> GetDirectoryEntries(const std::filesystem::path& dir);
	std::string WStringToUTF8(const std::wstring& wstr);

private:
	std::unique_ptr<UIInputLayer> m_pUILayer = nullptr;
	std::unique_ptr<FD3DW::SRVPacker> m_pUISRVPack;
	bool m_bIsInited = false;


private:
	//UI HELP FIELDS
	bool m_bShowSceneBrowser = false;
	bool m_bShowAudioBrowser = false;
	bool m_bShowSkyboxBrowser = false;
	bool m_bShowTextureBrowser = false;
	std::filesystem::path currentPath = SCENES_DEFAULT_PATH;
	std::vector<std::filesystem::directory_entry> entries = GetDirectoryEntries(currentPath);
	bool bPathChanged = false;
	

	struct TextureOpEntry {
		const char* Name;
		std::function<void(RenderableSimpleObject* obj, const std::string& path)> SetFunc;
		std::function<void(RenderableSimpleObject* obj)> EraseFunc;
	};

	std::vector<TextureOpEntry> m_vTextureOps;

	bool m_bShowTextureSelectPopup = false;
	int m_iTextureBeingSet = -1;
	RenderableSimpleObject* m_pTextureTargetObject = nullptr;


	struct AnimationUIState {
		int SelectedAnimIndex = 0;
		bool IsFreeze = false;
	};

	std::unordered_map<BaseRenderableObject*, AnimationUIState> m_mAnimationStates;

	int m_iSelectedObjectIndex = -1;
	std::vector<BaseRenderableObject*> m_vCachedObjects;


};
