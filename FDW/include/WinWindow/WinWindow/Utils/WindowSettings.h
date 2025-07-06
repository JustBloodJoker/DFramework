#pragma once

struct WindowSettings
{
	int Width = 600;
	int Height = 600;
	bool FullScreen = false;

	std::wstring TittleName = L"WinTest";

	bool IsResized;

	int MinWidth = 200;
	int MinHeight = 200;
};