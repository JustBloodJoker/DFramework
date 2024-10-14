#pragma once

struct WindowSettings
{
	int width = 600;
	int height = 600;
	bool fullScreen = false;

	std::wstring tittleName = L"WinTest";

	bool isResized;

	unsigned dlssWidth;
	unsigned dlssHeight;
};