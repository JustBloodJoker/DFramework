#include <RenderableObjects/RenderableSimpleCone.h>
#include <RenderableObjects/GeneratorsForSimpleObjects.h>

RenderableSimpleCone::RenderableSimpleCone() : RenderableSimpleObject("Simple cone") {}

std::unique_ptr<FD3DW::SimpleObject<FD3DW::SceneVertexFrameWork>> RenderableSimpleCone::CreateSimpleObject(ID3D12Device* device, ID3D12GraphicsCommandList* list) {
	return std::make_unique<FD3DW::SimpleObject<FD3DW::SceneVertexFrameWork>>(device, list, GenerateConeScene);
}
