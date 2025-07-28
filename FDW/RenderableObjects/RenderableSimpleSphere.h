#pragma once

#include <RenderableObjects/RenderableSimpleObject.h>

class RenderableSimpleSphere : public RenderableSimpleObject {
public:
	RenderableSimpleSphere();
	virtual ~RenderableSimpleSphere() = default;


public:

	BEGIN_FIELD_REGISTRATION(RenderableSimpleSphere, RenderableSimpleObject)
	END_FIELD_REGISTRATION()

protected:
	virtual std::unique_ptr<FD3DW::SimpleObject<FD3DW::SceneVertexFrameWork>> CreateSimpleObject(ID3D12Device* device, ID3D12GraphicsCommandList* list) override;
};