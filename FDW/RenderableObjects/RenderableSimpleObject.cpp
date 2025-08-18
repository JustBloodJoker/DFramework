#include <RenderableObjects/RenderableSimpleObject.h>
#include <MainRenderer/PSOManager.h>
#include <MainRenderer/GlobalTextureHeap.h>
#include <RenderableObjects/GeneratorsForSimpleObjects.h>


RenderableSimpleObject::RenderableSimpleObject(std::string name) :BaseRenderableObject(name) { 
	m_sName = name;
}

void RenderableSimpleObject::Init(ID3D12Device* device, ID3D12GraphicsCommandList* list) {
	m_pObject = CreateSimpleObject(device, list);

	m_pMatricesBuffer = FD3DW::UploadBuffer<MeshMatricesStructure>::CreateConstantBuffer(device, 1);

	m_pMaterialBuffer = FD3DW::UploadBuffer<MeshMaterialStructure>::CreateConstantBuffer(device, 1);

	auto cbvsrvuavsize = GetCBV_SRV_UAVDescriptorSize(device);

	m_pMaterial = std::make_unique<FD3DW::Material>();
	
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
			m_xMaterialCBufferData.LoadedTexture[i] = -1;
		}
	}

	UpdateWorldMatrix();
	NeedUpdateMaterials();
}

void RenderableSimpleObject::BeforeRender(const BeforeRenderInputData& data) {
	MeshMatricesStructure cmb;
	cmb.Projection = dx::XMMatrixTranspose(data.Projection);
	cmb.View = dx::XMMatrixTranspose(data.View);
	cmb.World = dx::XMMatrixTranspose(m_xWorldMatrix);
	cmb.IsActiveAnimation = false;
	cmb.CameraPosition = data.CameraPosition;

	m_pMatricesBuffer->CpyData(0, cmb);

	BeforeRenderMaterialsUpdate();
}

void RenderableSimpleObject::DeferredRender(ID3D12GraphicsCommandList* list) {
	//nothing to do
}

void RenderableSimpleObject::ForwardRender(ID3D12GraphicsCommandList* list) {
	//nothing to do
}

RenderPass RenderableSimpleObject::GetRenderPass() const {
	return RenderPass::Deferred;
}

void RenderableSimpleObject::InitBLASBuffers(ID3D12Device5* device, ID3D12GraphicsCommandList4* list)
{
	m_vBLASBuffers = FD3DW::CreateBLASForObject(device, list, m_pObject.get(), BASE_RENDERABLE_OBJECTS_BLAS_HIT_GROUP_INDEX, true);
}

bool RenderableSimpleObject::IsCanBeIndirectExecuted() {
	return true;
}

std::vector<std::pair<IndirectMeshRenderableData, InstanceData>> RenderableSimpleObject::GetDataToExecute() {
	IndirectMeshRenderableData data;
	data.CBMaterials = m_pMaterialBuffer->GetGPULocation(0);
	data.CBMatrices = m_pMatricesBuffer->GetGPULocation(0);
	data.SRVBones = GetEmptyStructuredBufferGPUVirtualAddress();
	data.VertexBufferView = *m_pObject->GetVertexBufferView();
	data.IndexBufferView = *m_pObject->GetIndexBufferView();
	auto params = m_pObject->GetObjectParameters(0);

	data.DrawArguments.IndexCountPerInstance = params.IndicesCount;
	data.DrawArguments.InstanceCount = 1u;
	data.DrawArguments.StartIndexLocation = params.IndicesOffset;
	data.DrawArguments.BaseVertexLocation = (INT)params.VerticesOffset;
	data.DrawArguments.StartInstanceLocation = 0u;

	InstanceData instanceData;

	auto [min, max] = GetBoundingBoxFromObjectDesc(m_pObject->GetObjectParameters(0), m_xWorldMatrix);
	instanceData.MinP = min;
	instanceData.MaxP = max;

	return { {data,instanceData} };
}

void RenderableSimpleObject::SetupTexture(FD3DW::TextureType type, std::string pathTo, ID3D12Device* device, ID3D12GraphicsCommandList* list) {
	m_pMaterial->SetTexture(pathTo, type, device, list);
	
	m_mPathToTextures[type] = pathTo;
	m_xMaterialCBufferData.LoadedTexture[type] = (int)GlobalTextureHeap::GetInstance()->AddTexture( m_pMaterial->GetTexture(type), device);
	m_xMaterialCBufferData.LoadedTexture[IS_ORM_TEXTURE_FLAG_POS] = m_pMaterial->IsORMTextureType();
	
	NeedUpdateMaterials();
}

void RenderableSimpleObject::EraseTexture(FD3DW::TextureType type, ID3D12Device* device) {
	
	auto texture = m_pMaterial->GetTexture(type);
	GlobalTextureHeap::GetInstance()->RemoveTexture(texture.get(), device);
	m_pMaterial->DeleteTexture(type);
	
	m_mPathToTextures.erase(type);
	
	m_xMaterialCBufferData.LoadedTexture[type] = -1;
	m_xMaterialCBufferData.LoadedTexture[IS_ORM_TEXTURE_FLAG_POS] = m_pMaterial->IsORMTextureType();
	
	NeedUpdateMaterials();
}

ID3D12Resource* RenderableSimpleObject::GetTexture(FD3DW::TextureType type) {
	return m_pMaterial->GetResourceTexture(type);
}

D3D12_GPU_DESCRIPTOR_HANDLE RenderableSimpleObject::GetTextureSRV(FD3DW::TextureType type) {
	return GlobalTextureHeap::GetInstance()->GetResult()->GetGPUDescriptorHandle( (UINT)GlobalTextureHeap::GetInstance()->GetIndex( m_pMaterial->GetTexture(type).get() ) );
}

void RenderableSimpleObject::BeforeRenderMaterialsUpdate() {
	if (m_bIsMaterialDataChanged) {
		m_pMaterialBuffer->CpyData(0, m_xMaterialCBufferData);
		m_bIsMaterialDataChanged = false;
	}

}

void RenderableSimpleObject::NeedUpdateMaterials() {
	m_bIsMaterialDataChanged = true;
}
