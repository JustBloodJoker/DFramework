#pragma once 

#include <pch.h>
#include <RenderableObjects/BaseRenderableObject.h>
#include <D3DFramework/GraphicUtilites/ResourcePacker.h>
#include <RenderableObjects/MeshMaterialStructure.h>
#include <RenderableObjects/MeshMatricesStructure.h>

struct RenderableMeshElementData {
	FD3DW::SRVPacker* SRVPack = nullptr;
	FD3DW::ObjectDesc ObjectDescriptor;
	MeshMaterialStructure MaterialCBufferData;
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
	virtual void DeferredRender(ID3D12GraphicsCommandList* list) override;
	virtual void ForwardRender(ID3D12GraphicsCommandList* list) override;
	virtual RenderPass GetRenderPass() const override;

	void SetAnimationPlaying(bool isP);

	////////////////////////////////////
	///////    MATERIAL SETTER INFO
	
	#define IMPL_XMFLOAT4_GET_SET_FUNCTION(name)  virtual dx::XMFLOAT4 Get##name() { return m_xData.MaterialCBufferData.name; }  virtual void Set##name(dx::XMFLOAT4 data) { m_xData.MaterialCBufferData.name = data; m_bIsMaterialDataChanged = true; }  
	#define IMPL_FLOAT_GET_SET_FUNCTION(name)  virtual float Get##name() { return m_xData.MaterialCBufferData.name; }  virtual void Set##name(float data) { m_xData.MaterialCBufferData.name = data; m_bIsMaterialDataChanged = true; }  

	IMPL_XMFLOAT4_GET_SET_FUNCTION(Diffuse);
	IMPL_XMFLOAT4_GET_SET_FUNCTION(Ambient);
	IMPL_XMFLOAT4_GET_SET_FUNCTION(Emissive);
	IMPL_XMFLOAT4_GET_SET_FUNCTION(Specular);

	IMPL_FLOAT_GET_SET_FUNCTION(Roughness);
	IMPL_FLOAT_GET_SET_FUNCTION(Metalness);
	IMPL_FLOAT_GET_SET_FUNCTION(SpecularPower);
	IMPL_FLOAT_GET_SET_FUNCTION(HeightScale);

	//////////////////////////////////////

private:

	std::unique_ptr<FD3DW::UploadBuffer<MeshMatricesStructure>> m_pMatricesBuffer;
	std::unique_ptr<FD3DW::UploadBuffer<MeshMaterialStructure>> m_pMaterialBuffer;

private:
	void BeforeRenderCBMaterial();

	////////////
	// FROM OWNER RENDERABLE
	RenderableMeshElementData m_xData;
	bool m_bIsMaterialDataChanged = false;
	///////////

private:
	bool m_bIsAnimationPlaying = false;

};
