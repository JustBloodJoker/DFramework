#pragma once

#include <D3DFramework/Utilites/Structures.h>

class SimpleMeshGeometry {
protected:
	static void GenerateSimplePlaneScene(std::vector<FD3DW::VertexFrameWork>& vertices, std::vector<std::uint32_t>& indices);
	static void GenerateSimpleConeScene(std::vector<FD3DW::VertexFrameWork>& vertices, std::vector<std::uint32_t>& indices);
	static void GenerateSimpleCubeScene(std::vector<FD3DW::VertexFrameWork>& vertices, std::vector<std::uint32_t>& indices);
	static void GenerateSimpleSphereScene(std::vector<FD3DW::VertexFrameWork>& vertices, std::vector<std::uint32_t>& indices);
	static void GenerateSimpleCylinderScene(std::vector<FD3DW::VertexFrameWork>& vertices, std::vector<std::uint32_t>& indices);
};
