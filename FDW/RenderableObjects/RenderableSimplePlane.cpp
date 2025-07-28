#include <RenderableObjects/RenderableSimplePlane.h>
#include <RenderableObjects/GeneratorsForSimpleObjects.h>

RenderableSimplePlane::RenderableSimplePlane() : RenderableSimpleObject("Simple plane") {}

std::unique_ptr<FD3DW::SimpleObject<FD3DW::SceneVertexFrameWork>> RenderableSimplePlane::CreateSimpleObject(ID3D12Device* device, ID3D12GraphicsCommandList* list) {
	return std::make_unique<FD3DW::SimpleObject<FD3DW::SceneVertexFrameWork>>(device, list, GenerateRectangleScene);
}
