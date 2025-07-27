#pragma once

#include <pch.h>
#include <MainRenderer/GlobalConfig.h>

std::function<void(std::vector<FD3DW::SceneVertexFrameWork>&, std::vector<std::uint32_t>&)> GetGeneratorForType(SimpleObjectType type);

void GenerateRectangleScene(std::vector<FD3DW::SceneVertexFrameWork>& vertices, std::vector<std::uint32_t>& indices);
