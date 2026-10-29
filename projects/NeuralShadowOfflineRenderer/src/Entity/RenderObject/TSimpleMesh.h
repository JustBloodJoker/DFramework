#pragma once

#include <Entity/RenderObject/TMesh.h>
#include <Entity/RenderObject/SimpleMeshGeometry.h>
#include <D3DFramework/Objects/SimpleObjects.h>
#include <D3DFramework/Objects/ObjectVertexIndexDataCreator.h>


class TSimpleMesh : public TMesh, protected SimpleMeshGeometry {
public:
	TSimpleMesh() = default;
	virtual ~TSimpleMesh() = default;

public:
    REFLECT_BODY(TSimpleMesh)
    BEGIN_REFLECT(TSimpleMesh, TMesh)
        REFLECT_PROPERTY(m_mPathToTextures)
    END_REFLECT(TSimpleMesh)

public:

	virtual void BeforeRenderInitAfterCreation(ID3D12Device* device, ID3D12GraphicsCommandList* list) override;
	virtual void BeforeRenderInitAfterLoad(ID3D12Device* device, ID3D12GraphicsCommandList* list) override;

	void SetDiffuse(dx::XMFLOAT4 data);

protected:
	void CallCreationObject(ID3D12Device* device, ID3D12GraphicsCommandList* list);
	virtual std::unique_ptr<FD3DW::SimpleObject<FD3DW::VertexFrameWork>> CreateSimpleObject(ID3D12Device* device, ID3D12GraphicsCommandList* list) = 0;

protected:
	std::map<FD3DW::TextureType, std::string> m_mPathToTextures;


    std::unique_ptr<FD3DW::ObjectVertexIndexDataCreator<FD3DW::VertexFrameWork>> m_pObjectVBV_IBV;
	std::unique_ptr<FD3DW::SimpleObject<FD3DW::VertexFrameWork>> m_pObject = nullptr;

	std::unique_ptr<FD3DW::Material> m_pMaterial = nullptr;
};
