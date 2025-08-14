#pragma once 

#include <pch.h>
#include <RenderableObjects/BaseRenderableObject.h>
#include <D3DFramework/GraphicUtilites/ResourcePacker.h>
#include <RenderableObjects/MeshMaterialStructure.h>
#include <RenderableObjects/MeshMatricesStructure.h>
#include <RenderableObjects/MeshIdxStructure.h>

struct RenderableMeshElementData {
	FD3DW::SRV_UAVPacker* SRVPack = nullptr;
	FD3DW::ObjectDesc ObjectDescriptor;
	MeshMaterialStructure MaterialCBufferData;
	size_t ID;

	BEGIN_FIELD_REGISTRATION(RenderableMeshElementData)
		REGISTER_FIELD(MaterialCBufferData);
		REGISTER_FIELD(ID);
	END_FIELD_REGISTRATION()
};



class RenderableMeshElement : public BaseRenderableObject {
private:
	static int __MeshElementsCreatedCount;

public:
	RenderableMeshElement();
	RenderableMeshElement(const RenderableMeshElementData& data);
	virtual ~RenderableMeshElement() = default;

public:
	void SetRenderableMeshElementData(const RenderableMeshElementData& data);

	virtual void BeforeDelete() override;

	virtual void Init(ID3D12Device* device, ID3D12GraphicsCommandList* list) override;
	virtual void BeforeRender(const BeforeRenderInputData& data) override;
	virtual void DeferredRender(ID3D12GraphicsCommandList* list) override;
	virtual void ForwardRender(ID3D12GraphicsCommandList* list) override;
	virtual RenderPass GetRenderPass() const override;

	void SetBonesIndex(int index);

	size_t ElementID();
	virtual void InitBLASBuffers(ID3D12Device5* device, ID3D12GraphicsCommandList4* list) override;
	void SetBLASBuffer(const FD3DW::AccelerationStructureBuffers& buffer);

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

	#undef IMPL_XMFLOAT4_GET_SET_FUNCTION
	#undef IMPL_FLOAT_GET_SET_FUNCTION

	//////////////////////////////////////

public:
	BEGIN_FIELD_REGISTRATION(RenderableMeshElement, BaseRenderableObject)
		REGISTER_FIELD(m_xData)
	END_FIELD_REGISTRATION()

private:
	std::unique_ptr<FD3DW::UploadBuffer<MeshBufferIndices>> m_pBufferIndicesBuffer;
	
private:
	UINT m_uMatricesIdxInBuffer = 0;
	UINT m_uMaterialIdxInBuffer = 0;

private:
	void BeforeRenderCBMaterial();

	////////////
	// FROM OWNER RENDERABLE
	RenderableMeshElementData m_xData;
	bool m_bIsMaterialDataChanged = false;
	///////////

private:
	int m_iStartIndexInBoneBuffer = -1;

};
