#pragma once

#include <D3DFramework/Objects/Scene.h>
#include <Entity/RenderObject/TMesh.h>
#include <D3DFramework/Objects/ObjectVertexIndexDataCreator.h>


class TScene : public TMesh {
public:
	TScene() = default;
	TScene(std::string path);
	virtual ~TScene() = default;


public:
    REFLECT_BODY(TScene)
    BEGIN_REFLECT(TScene, TMesh)
        REFLECT_PROPERTY(m_sPath)
    END_REFLECT(TScene)

public:
	virtual void BeforeRenderInitAfterCreation(ID3D12Device* device, ID3D12GraphicsCommandList* list) override;
	virtual void BeforeRenderInitAfterLoad(ID3D12Device* device, ID3D12GraphicsCommandList* list) override;

protected:
	static FD3DW::VertexFrameWork ConvertSceneVertex(FD3DW::SceneVertexFrameWork vertex);
	void CallCreationScene(ID3D12Device* device, ID3D12GraphicsCommandList* list);
protected:
	std::string m_sPath;
	std::unique_ptr<FD3DW::ObjectVertexIndexDataCreator<FD3DW::VertexFrameWork>> m_pObjectVBV_IBV;
	std::unique_ptr<FD3DW::Scene> m_pScene;


};
