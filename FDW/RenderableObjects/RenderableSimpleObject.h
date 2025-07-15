#pragma once 

#include <pch.h>
#include <D3DFramework/Objects/SimpleObjects.h>
#include <RenderableObjects/BaseRenderableObject.h>
#include <D3DFramework/GraphicUtilites/ResourcePacker.h>

class RenderableSimpleObject : public BaseRenderableObject {
public:
	RenderableSimpleObject(std::unique_ptr<FD3DW::SimpleObject> object);
	virtual ~RenderableSimpleObject() = default;

	//NOT IMPL

public:

	virtual void Init(ID3D12Device* device, ID3D12GraphicsCommandList* list) override;
	virtual void BeforeRender(const BeforeRenderInputData& data) override;
	virtual void DeferredRender(ID3D12GraphicsCommandList* list) override;
	virtual void ForwardRender(ID3D12GraphicsCommandList* list) override;
	virtual RenderPass GetRenderPass() const override;

private:
	std::unique_ptr<FD3DW::SimpleObject> m_pObject = nullptr;
};