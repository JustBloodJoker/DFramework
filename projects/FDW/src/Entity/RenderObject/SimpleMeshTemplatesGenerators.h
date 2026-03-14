#pragma once

#include <pch.h>
#include <MainRenderer/GlobalConfig.h>

void GenerateSimplePlaneScene(std::vector<FD3DW::VertexFrameWork>& vertices, std::vector<std::uint32_t>& indices);
void GenerateSimpleConeScene(std::vector<FD3DW::VertexFrameWork>& vertices, std::vector<std::uint32_t>& indices);
void GenerateSimpleCubeScene(std::vector<FD3DW::VertexFrameWork>& vertices, std::vector<std::uint32_t>& indices);
void GenerateSimpleSphereScene(std::vector<FD3DW::VertexFrameWork>& vertices, std::vector<std::uint32_t>& indices);