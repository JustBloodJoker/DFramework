#pragma once

#include <pch.h>
#include <MainRenderer/MainRendererComponent.h>
#include <UI/UIInputLayer.h>

class MainRenderer;
class BaseRenderableObject;

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



	void SceneBrowser();
	void ElementParamSetter();


private:
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
	std::filesystem::path currentPath = SCENES_DEFAULT_PATH;
	std::vector<std::filesystem::directory_entry> entries = GetDirectoryEntries(currentPath);
	bool bPathChanged = false;

	int m_iSelectedObjectIndex = -1;
	std::vector<BaseRenderableObject*> m_vCachedObjects;
};
