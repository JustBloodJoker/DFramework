#pragma once

#include <pch.h>
#include <MainRenderer/MainRendererComponent.h>
#include <UI/UIInputLayer.h>

class MainRenderer;

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
	std::unique_ptr<UIInputLayer> m_pUILayer = nullptr;
	std::unique_ptr<FD3DW::SRVPacker> m_pUISRVPack;
	bool m_bIsInited = false;
};
