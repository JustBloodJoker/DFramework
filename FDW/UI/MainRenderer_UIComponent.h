#pragma once

#include <pch.h>

class MainRenderer_UIComponent {
public:
	MainRenderer_UIComponent()=default;
	virtual ~MainRenderer_UIComponent() = default;

protected:

	void InitImGui(ID3D12Device* device, const HWND& hwnd, ID3D12DescriptorHeap* srvHeap);
	void RenderImGui(ID3D12GraphicsCommandList* cmdList);
	void ShutDownImGui();
	void ImGuiInputProcess(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void OnNewSizeWindowImGui(float width, float height);

private:
	void DrawUI();

private:
	bool m_bIsInited = false;
};
