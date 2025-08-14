#include <RenderableObjects/RenderableSimpleObject.h>
#include <MainRenderer/PSOManager.h>
#include <RenderableObjects/GeneratorsForSimpleObjects.h>


RenderableSimpleObject::RenderableSimpleObject(std::string name) :BaseRenderableObject(name) { }

void RenderableSimpleObject::BeforeDelete() {
	RenderableObjectsMatricesBuffer::GetInstance()->RemoveDataFromIndex(m_uMatricesIdxInBuffer);
	RenderableObjectsMaterialsBuffer::GetInstance()->RemoveDataFromIndex(m_uMaterialIdxInBuffer);
}

void RenderableSimpleObject::Init(ID3D12Device* device, ID3D12GraphicsCommandList* list) {
	m_pObject = CreateSimpleObject(device, list);

	m_pBufferIndicesBuffer = FD3DW::UploadBuffer<MeshBufferIndices>::CreateConstantBuffer(device, 1);

	auto cbvsrvuavsize = GetCBV_SRV_UAVDescriptorSize(device);

	m_pMaterial = std::make_unique<FD3DW::Material>();
	
	m_pSRVPack = FD3DW::SRV_UAVPacker::CreatePack(cbvsrvuavsize, TEXTURE_TYPE_SIZE, 0, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, device);
	
	if (!m_bIsInitedMaterialDesc) {
		m_xMaterialCBufferData.Diffuse = { 0.8f, 0.8f, 0.8f, 1.0f };
		m_xMaterialCBufferData.Ambient = { 0.1f, 0.1f, 0.1f, 1.0f };
		m_xMaterialCBufferData.Emissive = { 0.0f, 0.0f, 0.0f, 1.0f };
		m_xMaterialCBufferData.Specular = { 0.5f, 0.5f, 0.5f, 1.0f };
		m_xMaterialCBufferData.Roughness = 0.5f;
		m_xMaterialCBufferData.Metalness = 0.0f;
		m_xMaterialCBufferData.SpecularPower = 32.0f;
		m_xMaterialCBufferData.HeightScale = 0.04f;
		m_bIsInitedMaterialDesc = true;
	}

	for (int i = 0; i < TEXTURE_TYPE_SIZE; ++i)
	{
		auto type = FD3DW::TextureType(i);
		if (m_mPathToTextures.contains(type)) 
		{
			SetupTexture(type, m_mPathToTextures.at(type), device, list);
		} else 
		{
			m_xMaterialCBufferData.LoadedTexture[i] = false;
			m_pSRVPack->AddNullResource(i, device);
		}
	}


	m_uMatricesIdxInBuffer = RenderableObjectsMatricesBuffer::GetInstance()->GenerateIndexForBuffer();
	MeshMatricesStructure cmb;
	RenderableObjectsMatricesBuffer::GetInstance()->UploadDataToIndex(m_uMatricesIdxInBuffer, cmb);

	m_uMaterialIdxInBuffer = RenderableObjectsMaterialsBuffer::GetInstance()->GenerateIndexForBuffer();
	RenderableObjectsMaterialsBuffer::GetInstance()->UploadDataToIndex(m_uMaterialIdxInBuffer, m_xMaterialCBufferData);

	MeshBufferIndices indicesStruct;
	indicesStruct.MaterialIndex = m_uMaterialIdxInBuffer;
	indicesStruct.MatricesIndex = m_uMatricesIdxInBuffer;
	m_pBufferIndicesBuffer->CpyData(0, indicesStruct);

	UpdateWorldMatrix();
	NeedUpdateMaterials();
}

void RenderableSimpleObject::BeforeRender(const BeforeRenderInputData& data) {
	MeshMatricesStructure cmb;
	cmb.Projection = dx::XMMatrixTranspose(data.Projection);
	cmb.View = dx::XMMatrixTranspose(data.View);
	cmb.World = dx::XMMatrixTranspose(m_xWorldMatrix * data.AdditionalWorld);
	cmb.StartIndexInBoneBuffer = -1;
	cmb.CameraPosition = data.CameraPosition;

	RenderableObjectsMatricesBuffer::GetInstance()->UploadDataToIndex(m_uMatricesIdxInBuffer, cmb);

	BeforeRenderMaterialsUpdate();
}

void RenderableSimpleObject::DeferredRender(ID3D12GraphicsCommandList* list) {
	list->IASetVertexBuffers(0, 1, m_pObject->GetVertexBufferView());
	list->IASetIndexBuffer(m_pObject->GetIndexBufferView());

	ID3D12DescriptorHeap* heaps[] = { m_pSRVPack->GetResult()->GetDescriptorPtr() };
	list->SetDescriptorHeaps(_countof(heaps), heaps);
	list->SetGraphicsRootDescriptorTable(TEXTURE_START_POSITION_IN_ROOT_SIG, m_pSRVPack->GetResult()->GetGPUDescriptorHandle(0));
	list->SetGraphicsRootConstantBufferView(INDICES_CONSTANT_BUFFER_IN_ROOT_SIG, m_pBufferIndicesBuffer->GetGPULocation(0));

	auto params = m_pObject->GetObjectParameters(0);
	list->DrawIndexedInstanced(params.IndicesCount, 1u, params.IndicesOffset, params.VerticesOffset, 0u);
}

void RenderableSimpleObject::ForwardRender(ID3D12GraphicsCommandList* list) {
	//TODO
}

RenderPass RenderableSimpleObject::GetRenderPass() const {
	return RenderPass::Deferred;
}

void RenderableSimpleObject::InitBLASBuffers(ID3D12Device5* device, ID3D12GraphicsCommandList4* list)
{
	m_vBLASBuffers = FD3DW::CreateBLASForObject(device, list, m_pObject.get(), BASE_RENDERABLE_OBJECTS_BLAS_HIT_GROUP_INDEX, true);
}

void RenderableSimpleObject::SetupTexture(FD3DW::TextureType type, std::string pathTo, ID3D12Device* device, ID3D12GraphicsCommandList* list) {
	m_pMaterial->SetTexture(pathTo, type, device, list);
	m_pSRVPack->AddResource(m_pMaterial->GetResourceTexture(type), D3D12_SRV_DIMENSION_TEXTURE2D, type, device);

	m_mPathToTextures[type] = pathTo;
	m_xMaterialCBufferData.LoadedTexture[type] = true;
	m_xMaterialCBufferData.LoadedTexture[IS_ORM_TEXTURE_FLAG_POS] = m_pMaterial->IsORMTextureType();
	
	NeedUpdateMaterials();
}

void RenderableSimpleObject::EraseTexture(FD3DW::TextureType type, ID3D12Device* device) {
	m_pMaterial->DeleteTexture(type);
	m_pSRVPack->AddNullResource(type, device);

	m_mPathToTextures.erase(type);
	m_xMaterialCBufferData.LoadedTexture[type] = false;
	m_xMaterialCBufferData.LoadedTexture[IS_ORM_TEXTURE_FLAG_POS] = m_pMaterial->IsORMTextureType();
	
	NeedUpdateMaterials();
}

ID3D12Resource* RenderableSimpleObject::GetTexture(FD3DW::TextureType type) {
	return m_pMaterial->GetResourceTexture(type);
}

D3D12_GPU_DESCRIPTOR_HANDLE RenderableSimpleObject::GetTextureSRV(FD3DW::TextureType type) {
	return m_pSRVPack->GetResult()->GetGPUDescriptorHandle(type);
}

void RenderableSimpleObject::BeforeRenderMaterialsUpdate() {
	if (m_bIsMaterialDataChanged) {
		RenderableObjectsMaterialsBuffer::GetInstance()->UploadDataToIndex(m_uMaterialIdxInBuffer, m_xMaterialCBufferData);
		m_bIsMaterialDataChanged = false;
	}

}

void RenderableSimpleObject::NeedUpdateMaterials() {
	m_bIsMaterialDataChanged = true;
}
