#pragma once
#include "../pch.h"

namespace FDW
{

	struct WindowSettings
	{
		int width;
		int height;
		bool fullScreen;

		bool isResized;
	};

	struct VertexFrameWork
	{
		dx::XMFLOAT3 pos;
		dx::XMFLOAT3 normal;
		dx::XMFLOAT2 texCoord;
		dx::XMFLOAT3 tangent;
		dx::XMFLOAT3 bitangent;
	};

	struct MatricesConstantBufferStructureFrameWork
	{
		dx::XMMATRIX world;
		dx::XMMATRIX view;
		dx::XMMATRIX projection;
	};


}