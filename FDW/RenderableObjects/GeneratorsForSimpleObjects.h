#pragma once

#include <pch.h>
#include <MainRenderer/GlobalConfig.h>

void GenerateRectangleScene(std::vector<FD3DW::SceneVertexFrameWork>& vertices, std::vector<std::uint32_t>& indices);
void GenerateConeScene(std::vector<FD3DW::SceneVertexFrameWork>& vertices, std::vector<std::uint32_t>& indices);
void GenerateCubeScene(std::vector<FD3DW::SceneVertexFrameWork>& vertices, std::vector<std::uint32_t>& indices);
void GenerateSphereScene(std::vector<FD3DW::SceneVertexFrameWork>& vertices, std::vector<std::uint32_t>& indices);