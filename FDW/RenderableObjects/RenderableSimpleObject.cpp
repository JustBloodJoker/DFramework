#include <RenderableObjects/RenderableSimpleObject.h>


RenderableSimpleObject::RenderableSimpleObject(std::unique_ptr<FD3DW::SimpleObject> object) :BaseRenderableObject("Simple objects not impl!") {
	m_pObject = std::move(object);
	CONSOLE_ERROR_MESSAGE("RENDERABLE SIMPLE OBJECTS NOT IMPL");
}

void RenderableSimpleObject::Init(ID3D12Device* device, ID3D12GraphicsCommandList* list) {
	//TODO
}

void RenderableSimpleObject::BeforeRender(const BeforeRenderInputData& data) {
	//TODO
}

void RenderableSimpleObject::DeferredRender(ID3D12GraphicsCommandList* list) {
	//TODO
}

void RenderableSimpleObject::ForwardRender(ID3D12GraphicsCommandList* list) {
	//TODO
}

RenderPass RenderableSimpleObject::GetRenderPass() const {
	return RenderPass::Deferred;
}
