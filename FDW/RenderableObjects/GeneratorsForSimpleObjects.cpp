#include "GeneratorsForSimpleObjects.h"

void GenerateRectangleScene(std::vector<FD3DW::SceneVertexFrameWork>& vertices, std::vector<std::uint32_t>& indices)
{
	const dx::XMFLOAT3 normal = { 0.0f, 0.0f, 1.0f };
	const dx::XMFLOAT3 tangent = { 1.0f, 0.0f, 0.0f };
	const dx::XMFLOAT3 bitangent = { 0.0f, 1.0f, 0.0f }; 

	FD3DW::SceneVertexFrameWork v0;
	v0.Pos = { -1.0f, -1.0f, 0.0f };
	v0.Normal = normal;
	v0.TexCoord = { 0.0f, 1.0f };
	v0.Tangent = tangent;
	v0.Bitangent = bitangent;

	FD3DW::SceneVertexFrameWork v1;
	v1.Pos = { 1.0f, -1.0f, 0.0f };
	v1.Normal = normal;
	v1.TexCoord = { 1.0f, 1.0f };
	v1.Tangent = tangent;
	v1.Bitangent = bitangent;

	FD3DW::SceneVertexFrameWork v2;
	v2.Pos = { -1.0f,  1.0f, 0.0f };
	v2.Normal = normal;
	v2.TexCoord = { 0.0f, 0.0f };
	v2.Tangent = tangent;
	v2.Bitangent = bitangent;

	FD3DW::SceneVertexFrameWork v3;
	v3.Pos = { 1.0f,  1.0f, 0.0f };
	v3.Normal = normal;
	v3.TexCoord = { 1.0f, 0.0f };
	v3.Tangent = tangent;
	v3.Bitangent = bitangent;

	for (int i = 0; i < NUM_BONES_PER_VEREX; ++i)
	{
		v0.IDs[i] = v1.IDs[i] = v2.IDs[i] = v3.IDs[i] = 0;
		v0.Weights[i] = v1.Weights[i] = v2.Weights[i] = v3.Weights[i] = 0.0f;
	}

	vertices = { v0, v1, v2, v3 };
	indices = { 2, 1, 0, 3, 1, 2 };
}