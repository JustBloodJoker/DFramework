#include <Entity/RenderObject/SimpleMeshTemplatesGenerators.h>

void GenerateSimplePlaneScene(std::vector<FD3DW::SceneVertexFrameWork>& vertices, std::vector<std::uint32_t>& indices)
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

void GenerateSimpleConeScene(std::vector<FD3DW::SceneVertexFrameWork>& vertices, std::vector<std::uint32_t>& indices) {
	const int segments = 32;
	const float radius = 1.0f;
	const float height = 2.0f;
	const dx::XMFLOAT3 tangent = { 1.0f, 0.0f, 0.0f };
	const dx::XMFLOAT3 bitangent = { 0.0f, 0.0f, 1.0f };

	vertices.clear();
	indices.clear();

	FD3DW::SceneVertexFrameWork tipVertex;
	tipVertex.Pos = { 0.0f, height, 0.0f };
	tipVertex.Normal = { 0.0f, 1.0f, 0.0f };
	tipVertex.TexCoord = { 0.5f, 0.0f };
	tipVertex.Tangent = tangent;
	tipVertex.Bitangent = bitangent;

	for (int i = 0; i < NUM_BONES_PER_VEREX; ++i)
		tipVertex.IDs[i] = 0, tipVertex.Weights[i] = 0.0f;

	vertices.push_back(tipVertex);

	for (int i = 0; i <= segments; ++i) {
		float theta = (float)i / segments * M_2_PI_F;
		float x = radius * std::cos(theta);
		float z = radius * std::sin(theta);

		FD3DW::SceneVertexFrameWork baseVertex;
		baseVertex.Pos = { x, 0.0f, z };
		baseVertex.Normal = { x, radius / height, z };
		baseVertex.TexCoord = { (x / (2.0f * radius)) + 0.5f, (z / (2.0f * radius)) + 0.5f };
		baseVertex.Tangent = tangent;
		baseVertex.Bitangent = bitangent;

		for (int j = 0; j < NUM_BONES_PER_VEREX; ++j)
			baseVertex.IDs[j] = 0, baseVertex.Weights[j] = 0.0f;

		vertices.push_back(baseVertex);
	}

	for (int i = 1; i <= segments; ++i) {
		indices.push_back(0);
		indices.push_back(i);
		indices.push_back(i % segments + 1);
	}

	FD3DW::SceneVertexFrameWork centerVertex;
	centerVertex.Pos = { 0.0f, 0.0f, 0.0f };
	centerVertex.Normal = { 0.0f, -1.0f, 0.0f };
	centerVertex.TexCoord = { 0.5f, 0.5f };
	centerVertex.Tangent = tangent;
	centerVertex.Bitangent = bitangent;

	for (int j = 0; j < NUM_BONES_PER_VEREX; ++j)
		centerVertex.IDs[j] = 0, centerVertex.Weights[j] = 0.0f;

	int centerIndex = static_cast<int>(vertices.size());
	vertices.push_back(centerVertex);

	for (int i = 1; i <= segments; ++i) {
		indices.push_back(centerIndex);
		indices.push_back(i % segments + 1);
		indices.push_back(i);
	}
}

void GenerateSimpleCubeScene(std::vector<FD3DW::SceneVertexFrameWork>& vertices, std::vector<std::uint32_t>& indices) {
	vertices.clear();
	indices.clear();

	const dx::XMFLOAT3 normals[6] = {
		{  0.0f,  0.0f,  1.0f },
		{  0.0f,  0.0f, -1.0f },
		{ -1.0f,  0.0f,  0.0f },
		{  1.0f,  0.0f,  0.0f },
		{  0.0f,  1.0f,  0.0f },
		{  0.0f, -1.0f,  0.0f }
	};

	const dx::XMFLOAT3 tangents[6] = {
		{ 1.0f, 0.0f, 0.0f },
		{-1.0f, 0.0f, 0.0f },
		{ 0.0f, 0.0f,-1.0f },
		{ 0.0f, 0.0f, 1.0f },
		{ 1.0f, 0.0f, 0.0f },
		{ 1.0f, 0.0f, 0.0f }
	};

	const dx::XMFLOAT3 bitangents[6] = {
		{ 0.0f, 1.0f, 0.0f },
		{ 0.0f, 1.0f, 0.0f },
		{ 0.0f, 1.0f, 0.0f },
		{ 0.0f, 1.0f, 0.0f },
		{ 0.0f, 0.0f, -1.0f },
		{ 0.0f, 0.0f,  1.0f }
	};

	const dx::XMFLOAT3 faceVertices[6][4] = {
		{ {-1,-1, 1}, {1,-1, 1}, {-1,1, 1}, {1,1, 1} },
		{ {1,-1,-1}, {-1,-1,-1}, {1,1,-1}, {-1,1,-1} },
		{ {-1,-1,-1}, {-1,-1, 1}, {-1,1,-1}, {-1,1,1} },
		{ {1,-1, 1}, {1,-1,-1}, {1,1, 1}, {1,1,-1} },
		{ {-1,1, 1}, {1,1, 1}, {-1,1,-1}, {1,1,-1} },
		{ {-1,-1,-1}, {1,-1,-1}, {-1,-1, 1}, {1,-1,1} }
	};

	const dx::XMFLOAT2 texCoords[4] = {
		{0.0f, 1.0f},
		{1.0f, 1.0f},
		{0.0f, 0.0f},
		{1.0f, 0.0f}
	};

	for (int face = 0; face < 6; ++face) {
		int baseIndex = static_cast<int>(vertices.size());

		for (int i = 0; i < 4; ++i) {
			FD3DW::SceneVertexFrameWork v;
			v.Pos = faceVertices[face][i];
			v.Normal = normals[face];
			v.Tangent = tangents[face];
			v.Bitangent = bitangents[face];
			v.TexCoord = texCoords[i];

			for (int j = 0; j < NUM_BONES_PER_VEREX; ++j) {
				v.IDs[j] = 0;
				v.Weights[j] = 0.0f;
			}

			vertices.push_back(v);
		}

		indices.push_back(baseIndex + 0);
		indices.push_back(baseIndex + 1);
		indices.push_back(baseIndex + 2);

		indices.push_back(baseIndex + 2);
		indices.push_back(baseIndex + 1);
		indices.push_back(baseIndex + 3);
	}
}

void GenerateSimpleSphereScene(std::vector<FD3DW::SceneVertexFrameWork>& vertices, std::vector<std::uint32_t>& indices) {
	const int latitudeBands = 32;
	const int longitudeBands = 32;
	const float radius = 1.0f;

	vertices.clear();
	indices.clear();

	for (int lat = 0; lat <= latitudeBands; ++lat) {
		float theta = lat * M_PI_F / latitudeBands;
		float sinTheta = std::sin(theta);
		float cosTheta = std::cos(theta);

		for (int lon = 0; lon <= longitudeBands; ++lon) {
			float phi = lon * 2.0f * M_PI_F / longitudeBands;
			float sinPhi = std::sin(phi);
			float cosPhi = std::cos(phi);

			float x = cosPhi * sinTheta;
			float y = cosTheta;
			float z = sinPhi * sinTheta;

			dx::XMFLOAT3 position = { radius * x, radius * y, radius * z };
			dx::XMFLOAT3 normal = { x, y, z };
			dx::XMFLOAT2 texCoord = { 1.0f - ((float)lon / longitudeBands), 1.0f - ((float)lat / latitudeBands) };

			dx::XMFLOAT3 tangent = { -sinPhi, 0.0f, cosPhi };
			dx::XMFLOAT3 bitangent = {
				cosTheta * cosPhi,
				-sinTheta,
				cosTheta * sinPhi
			};

			FD3DW::SceneVertexFrameWork v;
			v.Pos = position;
			v.Normal = normal;
			v.TexCoord = texCoord;
			v.Tangent = tangent;
			v.Bitangent = bitangent;

			for (int j = 0; j < NUM_BONES_PER_VEREX; ++j) {
				v.IDs[j] = 0;
				v.Weights[j] = 0.0f;
			}

			vertices.push_back(v);
		}
	}

	for (int lat = 0; lat < latitudeBands; ++lat) {
		for (int lon = 0; lon < longitudeBands; ++lon) {
			int first = (lat * (longitudeBands + 1)) + lon;
			int second = first + longitudeBands + 1;

			indices.push_back(first);
			indices.push_back(second);
			indices.push_back(first + 1);

			indices.push_back(second);
			indices.push_back(second + 1);
			indices.push_back(first + 1);
		}
	}
}

