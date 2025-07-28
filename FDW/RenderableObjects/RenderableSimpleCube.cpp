#include <RenderableObjects/RenderableSimpleCube.h>
#include <RenderableObjects/GeneratorsForSimpleObjects.h>

RenderableSimpleCube::RenderableSimpleCube() : RenderableSimpleObject("Simple cube") {}

std::unique_ptr<FD3DW::SimpleObject<FD3DW::SceneVertexFrameWork>> RenderableSimpleCube::CreateSimpleObject(ID3D12Device* device, ID3D12GraphicsCommandList* list) {
	return std::make_unique<FD3DW::SimpleObject<FD3DW::SceneVertexFrameWork>>(device, list, GenerateCubeScene);
}
