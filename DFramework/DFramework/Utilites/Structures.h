#pragma once
#include "../pch.h"

namespace FDW
{

	struct WindowSettings
	{
		int width;
		int height;
		bool fullScreen;

		std::wstring tittleName;

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

	struct SceneVertexFrameWork
	{
		dx::XMFLOAT3 pos;
		dx::XMFLOAT3 normal;
		dx::XMFLOAT2 texCoord;
		dx::XMFLOAT3 tangent;
		dx::XMFLOAT3 bitangent; 
		UINT IDs[NUM_BONES_PER_VEREX];
		float Weights[NUM_BONES_PER_VEREX];
	};

	struct MatricesConstantBufferStructureFrameWork
	{
		dx::XMMATRIX world;
		dx::XMMATRIX view;
		dx::XMMATRIX projection;
	};

	struct AnimationParams
	{
		std::vector<std::unique_ptr<aiNodeAnim>> channels;
		float duration;
	};

}