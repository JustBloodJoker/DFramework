#pragma once

#include <pch.h>
#include <RenderableObjects/BaseRenderableObject.h>
#include <D3DFramework/Objects/SimpleObjects.h>
#include <D3DFramework/GraphicUtilites/ResourcePacker.h>

class RenderableSkyboxObject : public BaseRenderableObject {
public:
	RenderableSkyboxObject() = default;
	RenderableSkyboxObject(const std::string& pathToSkybox);
	virtual ~RenderableSkyboxObject() = default;

	virtual void Init(ID3D12Device* device, ID3D12GraphicsCommandList* list) override;
	virtual void BeforeRender(const BeforeRenderInputData& data) override;
	virtual void DeferredRender(ID3D12GraphicsCommandList* list) override;
	virtual void ForwardRender(ID3D12GraphicsCommandList* list) override;
	virtual void PreDepthRender(ID3D12GraphicsCommandList* list) override;
	virtual RenderPass GetRenderPass() const override;

	virtual void InitBLASBuffers(ID3D12Device5* device, ID3D12GraphicsCommandList4* list) override;
public:
	BEGIN_FIELD_REGISTRATION(RenderableSkyboxObject, BaseRenderableObject)
		REGISTER_FIELD(m_sTexturePath);
	END_FIELD_REGISTRATION()

protected:
	std::string m_sTexturePath;
	std::unique_ptr<FD3DW::SRV_UAVPacker> m_pSRVPack;
	std::unique_ptr<FD3DW::Material> m_pMaterial;
	std::unique_ptr<FD3DW::Cube> m_pCube;
	std::unique_ptr<FD3DW::UploadBuffer<FD3DW::MatricesConstantBufferStructureFrameWork>> m_pMatricesBuffer;
};