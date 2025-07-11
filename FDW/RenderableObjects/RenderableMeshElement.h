#pragma once 

#include <pch.h>
#include <RenderableObjects/BaseRenderableObject.h>
#include <D3DFramework/GraphicUtilites/ResourcePacker.h>

struct RenderableMeshElementData {
	FD3DW::SamplerPacker* SamplerPack = nullptr;
	FD3DW::SRVPacker* SRVPack = nullptr;
	FD3DW::ObjectDesc ObjectDescriptor;

};


class RenderableMeshElement : public BaseRenderableObject {
private:
	static int __MeshElementsCreatedCount;

public:
	RenderableMeshElement(const RenderableMeshElementData& data);
	virtual ~RenderableMeshElement() = default;

public:
	virtual void Init(ID3D12Device* device, ID3D12GraphicsCommandList* list) override;
	virtual void BeforeRender(const BeforeRenderInputData& data) override;
	virtual void Render(ID3D12GraphicsCommandList* list) override;

private:

	std::unique_ptr<FD3DW::UploadBuffer<FD3DW::MatricesConstantBufferStructureFrameWork>> m_pMatricesBuffer;

private:
	////////////
	// FROM OWNER RENDERABLE
	RenderableMeshElementData m_xData;

	///////////

};
