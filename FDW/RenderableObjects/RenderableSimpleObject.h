#pragma once 

#include <pch.h>
#include <D3DFramework/Objects/SimpleObjects.h>
#include <RenderableObjects/BaseRenderableObject.h>
#include <RenderableObjects/IndirectExecutionMeshObject.h>
#include <D3DFramework/GraphicUtilites/ResourcePacker.h>
#include <RenderableObjects/MeshMaterialStructure.h>
#include <RenderableObjects/MeshMatricesStructure.h>



class RenderableSimpleObject : virtual public BaseRenderableObject, virtual  public IndirectExecutionMeshObject {
private:


public:
	RenderableSimpleObject() = default;
	RenderableSimpleObject(std::string name);
	virtual ~RenderableSimpleObject() = default;

public:

	virtual void Init(ID3D12Device* device, ID3D12GraphicsCommandList* list) override;
	virtual void BeforeRender(const BeforeRenderInputData& data) override;
	virtual void DeferredRender(ID3D12GraphicsCommandList* list) override;
	virtual void ForwardRender(ID3D12GraphicsCommandList* list) override;
	virtual void PreDepthRender(ID3D12GraphicsCommandList* list) override;
	virtual RenderPass GetRenderPass() const override;
	
	virtual void InitBLASBuffers(ID3D12Device5* device, ID3D12GraphicsCommandList4* list) override;

	virtual bool IsCanBeIndirectExecuted() override;
	virtual std::vector<std::pair<IndirectMeshRenderableData, InstanceData>> GetDataToExecute() override;

	//////////////////////////////////////////////////
	//////   MATERIAL DATA & TEXTURE GET SET
	
	#define IMPL_XMFLOAT4_GET_SET_FUNCTION(name)  virtual dx::XMFLOAT4 Get##name() { return m_xMaterialCBufferData.name; }  virtual void Set##name(dx::XMFLOAT4 data) { m_xMaterialCBufferData.name = data; NeedUpdateMaterials(); }  
	#define IMPL_FLOAT_GET_SET_FUNCTION(name)  virtual float Get##name() { return m_xMaterialCBufferData.name; }  virtual void Set##name(float data) { m_xMaterialCBufferData.name = data; NeedUpdateMaterials(); }  
	
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

	//////////////////////////////////////////////////


	void SetupTexture(FD3DW::TextureType type, std::string pathTo, ID3D12Device* device, ID3D12GraphicsCommandList* list);
	void EraseTexture(FD3DW::TextureType type, ID3D12Device* device);
	ID3D12Resource* GetTexture(FD3DW::TextureType type);
	D3D12_GPU_DESCRIPTOR_HANDLE GetTextureSRV(FD3DW::TextureType type);


public:

	BEGIN_FIELD_REGISTRATION(RenderableSimpleObject, BaseRenderableObject)
		REGISTER_FIELD(m_xMaterialCBufferData)
		REGISTER_FIELD(m_bIsInitedMaterialDesc)
		REGISTER_FIELD(m_mPathToTextures)
	END_FIELD_REGISTRATION()

protected:
	virtual std::unique_ptr<FD3DW::SimpleObject<FD3DW::SceneVertexFrameWork>> CreateSimpleObject(ID3D12Device* device, ID3D12GraphicsCommandList* list) = 0;

private:
	void BeforeRenderMaterialsUpdate();
	void NeedUpdateMaterials();

private:
	std::map<FD3DW::TextureType, std::string> m_mPathToTextures;

	std::unique_ptr<FD3DW::SimpleObject<FD3DW::SceneVertexFrameWork>> m_pObject = nullptr;
	
	std::unique_ptr<FD3DW::Material> m_pMaterial = nullptr;

	std::unique_ptr<FD3DW::UploadBuffer<MeshMatricesStructure>> m_pMatricesBuffer;
	std::unique_ptr<FD3DW::UploadBuffer<MeshMaterialStructure>> m_pMaterialBuffer;

private:
	bool m_bIsInitedMaterialDesc = false;
	bool m_bIsMaterialDataChanged = false;
	MeshMaterialStructure m_xMaterialCBufferData;
};