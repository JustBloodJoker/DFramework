#include <Entity/RenderObject/SimpleMeshGeometry.h>

void SimpleMeshGeometry::GenerateSimplePlaneScene(std::vector<FD3DW::VertexFrameWork>& vertices, std::vector<std::uint32_t>& indices)
{
	const dx::XMFLOAT3 normal = { 0.0f, 0.0f, 1.0f };
	const dx::XMFLOAT3 tangent = { 1.0f, 0.0f, 0.0f };
	const dx::XMFLOAT3 bitangent = { 0.0f, 1.0f, 0.0f };

	FD3DW::VertexFrameWork bottomLeft;
	bottomLeft.Pos = { -1.0f, -1.0f, 0.0f };
	bottomLeft.Normal = normal;
	bottomLeft.TexCoord = { 0.0f, 1.0f };
	bottomLeft.Tangent = tangent;
	bottomLeft.Bitangent = bitangent;

	FD3DW::VertexFrameWork bottomRight;
	bottomRight.Pos = { 1.0f, -1.0f, 0.0f };
	bottomRight.Normal = normal;
	bottomRight.TexCoord = { 1.0f, 1.0f };
	bottomRight.Tangent = tangent;
	bottomRight.Bitangent = bitangent;

	FD3DW::VertexFrameWork topLeft;
	topLeft.Pos = { -1.0f, 1.0f, 0.0f };
	topLeft.Normal = normal;
	topLeft.TexCoord = { 0.0f, 0.0f };
	topLeft.Tangent = tangent;
	topLeft.Bitangent = bitangent;

	FD3DW::VertexFrameWork topRight;
	topRight.Pos = { 1.0f, 1.0f, 0.0f };
	topRight.Normal = normal;
	topRight.TexCoord = { 1.0f, 0.0f };
	topRight.Tangent = tangent;
	topRight.Bitangent = bitangent;

	vertices = { bottomLeft, bottomRight, topLeft, topRight };
	indices = { 2, 1, 0, 3, 1, 2 };
}

void SimpleMeshGeometry::GenerateSimpleConeScene(std::vector<FD3DW::VertexFrameWork>& vertices, std::vector<std::uint32_t>& indices)
{
	constexpr int segments = 32;
	constexpr float radius = 1.0f;
	constexpr float height = 2.0f;
	const dx::XMFLOAT3 tangent = { 1.0f, 0.0f, 0.0f };
	const dx::XMFLOAT3 bitangent = { 0.0f, 0.0f, 1.0f };

	vertices.clear();
	indices.clear();

	FD3DW::VertexFrameWork tipVertex{};
	tipVertex.Pos = { 0.0f, height, 0.0f };
	tipVertex.Normal = { 0.0f, 1.0f, 0.0f };
	tipVertex.TexCoord = { 0.5f, 0.0f };
	tipVertex.Tangent = tangent;
	tipVertex.Bitangent = bitangent;
	vertices.push_back(tipVertex);

	for (int i = 0; i <= segments; ++i) {
		const float theta = static_cast<float>(i) / segments * M_2_PI_F;
		const float x = radius * std::cos(theta);
		const float z = radius * std::sin(theta);
		FD3DW::VertexFrameWork baseVertex{};
		baseVertex.Pos = { x, 0.0f, z };
		baseVertex.Normal = { x, radius / height, z };
		baseVertex.TexCoord = { x / (2.0f * radius) + 0.5f, z / (2.0f * radius) + 0.5f };
		baseVertex.Tangent = tangent;
		baseVertex.Bitangent = bitangent;
		vertices.push_back(baseVertex);
	}

	for (int i = 1; i <= segments; ++i) {
		indices.push_back(0);
		indices.push_back(i);
		indices.push_back(i % segments + 1);
	}

	FD3DW::VertexFrameWork centerVertex{};
	centerVertex.Pos = { 0.0f, 0.0f, 0.0f };
	centerVertex.Normal = { 0.0f, -1.0f, 0.0f };
	centerVertex.TexCoord = { 0.5f, 0.5f };
	centerVertex.Tangent = tangent;
	centerVertex.Bitangent = bitangent;
	const auto centerIndex = static_cast<std::uint32_t>(vertices.size());
	vertices.push_back(centerVertex);

	for (int i = 1; i <= segments; ++i) {
		indices.push_back(centerIndex);
		indices.push_back(i % segments + 1);
		indices.push_back(i);
	}
}

void SimpleMeshGeometry::GenerateSimpleCubeScene(std::vector<FD3DW::VertexFrameWork>& vertices, std::vector<std::uint32_t>& indices)
{
	vertices.clear();
	indices.clear();
	const dx::XMFLOAT3 normals[6] = {
		{ 0, 0, 1 }, { 0, 0, -1 }, { -1, 0, 0 }, { 1, 0, 0 }, { 0, 1, 0 }, { 0, -1, 0 }
	};
	const dx::XMFLOAT3 tangents[6] = {
		{ 1, 0, 0 }, { -1, 0, 0 }, { 0, 0, -1 }, { 0, 0, 1 }, { 1, 0, 0 }, { 1, 0, 0 }
	};
	const dx::XMFLOAT3 bitangents[6] = {
		{ 0, 1, 0 }, { 0, 1, 0 }, { 0, 1, 0 }, { 0, 1, 0 }, { 0, 0, -1 }, { 0, 0, 1 }
	};
	const dx::XMFLOAT3 faceVertices[6][4] = {
		{{-1,-1,1},{1,-1,1},{-1,1,1},{1,1,1}}, {{1,-1,-1},{-1,-1,-1},{1,1,-1},{-1,1,-1}},
		{{-1,-1,-1},{-1,-1,1},{-1,1,-1},{-1,1,1}}, {{1,-1,1},{1,-1,-1},{1,1,1},{1,1,-1}},
		{{-1,1,1},{1,1,1},{-1,1,-1},{1,1,-1}}, {{-1,-1,-1},{1,-1,-1},{-1,-1,1},{1,-1,1}}
	};
	const dx::XMFLOAT2 texCoords[4] = {{0,1},{1,1},{0,0},{1,0}};

	for (int face = 0; face < 6; ++face) {
		const auto baseIndex = static_cast<std::uint32_t>(vertices.size());
		for (int i = 0; i < 4; ++i) {
			FD3DW::VertexFrameWork vertex{};
			vertex.Pos = faceVertices[face][i];
			vertex.Normal = normals[face];
			vertex.Tangent = tangents[face];
			vertex.Bitangent = bitangents[face];
			vertex.TexCoord = texCoords[i];
			vertices.push_back(vertex);
		}
		indices.insert(indices.end(), { baseIndex, baseIndex + 1, baseIndex + 2,
			baseIndex + 2, baseIndex + 1, baseIndex + 3 });
	}
}

void SimpleMeshGeometry::GenerateSimpleSphereScene(std::vector<FD3DW::VertexFrameWork>& vertices, std::vector<std::uint32_t>& indices)
{
	constexpr int latitudeBands = 32;
	constexpr int longitudeBands = 32;
	constexpr float radius = 1.0f;
	vertices.clear();
	indices.clear();

	for (int latitude = 0; latitude <= latitudeBands; ++latitude) {
		const float theta = latitude * M_PI_F / latitudeBands;
		const float sinTheta = std::sin(theta);
		const float cosTheta = std::cos(theta);
		for (int longitude = 0; longitude <= longitudeBands; ++longitude) {
			const float phi = longitude * 2.0f * M_PI_F / longitudeBands;
			const float sinPhi = std::sin(phi);
			const float cosPhi = std::cos(phi);
			const float x = cosPhi * sinTheta;
			const float y = cosTheta;
			const float z = sinPhi * sinTheta;
			FD3DW::VertexFrameWork vertex{};
			vertex.Pos = { radius * x, radius * y, radius * z };
			vertex.Normal = { x, y, z };
			vertex.TexCoord = { 1.0f - static_cast<float>(longitude) / longitudeBands,
				1.0f - static_cast<float>(latitude) / latitudeBands };
			vertex.Tangent = { -sinPhi, 0.0f, cosPhi };
			vertex.Bitangent = { cosTheta * cosPhi, -sinTheta, cosTheta * sinPhi };
			vertices.push_back(vertex);
		}
	}

	for (int latitude = 0; latitude < latitudeBands; ++latitude) {
		for (int longitude = 0; longitude < longitudeBands; ++longitude) {
			const auto first = static_cast<std::uint32_t>(latitude * (longitudeBands + 1) + longitude);
			const auto second = first + longitudeBands + 1;
			indices.insert(indices.end(), { first, second, first + 1, second, second + 1, first + 1 });
		}
	}
}

void SimpleMeshGeometry::GenerateSimpleCylinderScene(std::vector<FD3DW::VertexFrameWork>& vertices, std::vector<std::uint32_t>& indices)
{
	constexpr int segments = 32;
	constexpr float radius = 1.0f;
	constexpr float halfHeight = 1.0f;
	vertices.clear();
	indices.clear();

	for (int segment = 0; segment <= segments; ++segment) {
		const float angle = static_cast<float>(segment) / segments * M_2_PI_F;
		const float x = std::cos(angle);
		const float z = std::sin(angle);
		for (int top = 0; top < 2; ++top) {
			FD3DW::VertexFrameWork vertex{};
			vertex.Pos = { radius * x, top == 0 ? -halfHeight : halfHeight, radius * z };
			vertex.Normal = { x, 0.0f, z };
			vertex.TexCoord = { static_cast<float>(segment) / segments, static_cast<float>(top) };
			vertex.Tangent = { -z, 0.0f, x };
			vertex.Bitangent = { 0.0f, 1.0f, 0.0f };
			vertices.push_back(vertex);
		}
	}

	for (int segment = 0; segment < segments; ++segment) {
		const auto base = static_cast<std::uint32_t>(segment * 2);
		indices.insert(indices.end(), { base, base + 1, base + 2, base + 2, base + 1, base + 3 });
	}

	for (int top = 0; top < 2; ++top) {
		const auto center = static_cast<std::uint32_t>(vertices.size());
		FD3DW::VertexFrameWork centerVertex{};
		centerVertex.Pos = { 0.0f, top == 0 ? -halfHeight : halfHeight, 0.0f };
		centerVertex.Normal = { 0.0f, top == 0 ? -1.0f : 1.0f, 0.0f };
		centerVertex.TexCoord = { 0.5f, 0.5f };
		centerVertex.Tangent = { 1.0f, 0.0f, 0.0f };
		centerVertex.Bitangent = { 0.0f, 0.0f, 1.0f };
		vertices.push_back(centerVertex);
		for (int segment = 0; segment < segments; ++segment) {
			const float angle0 = static_cast<float>(segment) / segments * M_2_PI_F;
			const float angle1 = static_cast<float>(segment + 1) / segments * M_2_PI_F;
			const auto first = static_cast<std::uint32_t>(vertices.size());
			for (const float angle : { angle0, angle1 }) {
				FD3DW::VertexFrameWork edge{};
				edge.Pos = { radius * std::cos(angle), top == 0 ? -halfHeight : halfHeight, radius * std::sin(angle) };
				edge.Normal = centerVertex.Normal;
				edge.TexCoord = { 0.5f + 0.5f * std::cos(angle), 0.5f + 0.5f * std::sin(angle) };
				edge.Tangent = centerVertex.Tangent;
				edge.Bitangent = centerVertex.Bitangent;
				vertices.push_back(edge);
			}
			if (top == 0) {
				indices.insert(indices.end(), { center, first + 1, first });
			} else {
				indices.insert(indices.end(), { center, first, first + 1 });
			}
		}
	}
}
