#pragma once

#include <pch.h>
#include <Component/RenderObject/RenderComponent.h>
#include <D3DFramework/Objects/SimpleObjects.h>
#include <D3DFramework/Objects/ObjectVertexIndexDataCreator.h>
#include <D3DFramework/GraphicUtilites/ResourcePacker.h>

class SkyboxComponent : public RenderComponent {
public:
	SkyboxComponent() = default;
	SkyboxComponent(std::string path);
	virtual ~SkyboxComponent() = default;


public:

    REFLECT_BODY(SkyboxComponent)
    BEGIN_REFLECT(SkyboxComponent, RenderComponent)
        REFLECT_PROPERTY(m_sPathToTexture)
    END_REFLECT(SkyboxComponent)


public:
	virtual void OnStartRenderTick(const RenderComponentBeforeRenderInputData& data) override;
	virtual void RenderInit(ID3D12Device* device, ID3D12GraphicsCommandList* list) override;
	virtual void OnEndRenderTick(ID3D12GraphicsCommandList* list) override;

public:
	virtual void Init() override;
	virtual void Destroy() override;
	virtual void Activate(bool a) override;

public:
	std::string PathToTexture() const;
	FD3DW::SRV_UAVPacker* SRVPack() const;
	FD3DW::Material* Material() const;
	FD3DW::Cube* Cube() const;
	FD3DW::IObjectVertexIndexDataCreator* CubeVBV_IBV() const;
	FD3DW::MatricesConstantBufferStructureFrameWork MatricesData();

protected:
	std::string m_sPathToTexture;
	FD3DW::MatricesConstantBufferStructureFrameWork m_xMatricesData;
	std::unique_ptr<FD3DW::SRV_UAVPacker> m_pSRVPack;
	std::unique_ptr<FD3DW::Material> m_pMaterial;
	std::unique_ptr<FD3DW::Cube> m_pCube;
	std::unique_ptr<FD3DW::ObjectVertexIndexDataCreator<FD3DW::VertexFrameWork>> m_pObjectVBV_IBV;

};