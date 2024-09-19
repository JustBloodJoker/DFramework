#pragma once

struct WindowSettings
{
	int width;
	int height;
	bool fullScreen;

	std::wstring tittleName;

	bool isResized;

	unsigned dlssWidth;
	unsigned dlssHeight;
};