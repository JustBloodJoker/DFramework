#pragma once
#include "../pch.h"

namespace FD3DW
{
	struct VertexFrameWork
	{
		dx::XMFLOAT3 Pos;
		dx::XMFLOAT3 Normal;
		dx::XMFLOAT2 TexCoord;
		dx::XMFLOAT3 Tangent;
		dx::XMFLOAT3 Bitangent;
	};

	struct SceneVertexFrameWork
	{
		dx::XMFLOAT3 Pos;
		dx::XMFLOAT3 Normal;
		dx::XMFLOAT2 TexCoord;
		dx::XMFLOAT3 Tangent;
		dx::XMFLOAT3 Bitangent; 
		UINT IDs[NUM_BONES_PER_VEREX];
		float Weights[NUM_BONES_PER_VEREX];
	};

	struct MatricesConstantBufferStructureFrameWork
	{
		dx::XMMATRIX World;
		dx::XMMATRIX View;
		dx::XMMATRIX Projection;
	};

	struct AnimationParams
	{
		std::vector<std::unique_ptr<aiNodeAnim>> Channels;
		float Duration;
	};

}