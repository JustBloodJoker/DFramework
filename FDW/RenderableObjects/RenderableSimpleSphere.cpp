#include <RenderableObjects/RenderableSimpleSphere.h>
#include <RenderableObjects/GeneratorsForSimpleObjects.h>

RenderableSimpleSphere::RenderableSimpleSphere() : RenderableSimpleObject("Simple sphere") {}

std::unique_ptr<FD3DW::SimpleObject<FD3DW::SceneVertexFrameWork>> RenderableSimpleSphere::CreateSimpleObject(ID3D12Device* device, ID3D12GraphicsCommandList* list) {
	return std::make_unique<FD3DW::SimpleObject<FD3DW::SceneVertexFrameWork>>(device, list, GenerateSphereScene);
}
