#include <Entity/RenderObject/TSimpleMesh.h>
#include <MainRenderer/GlobalTextureHeap.h>
#include <World/World.h>
#include <D3DFramework/Objects/ObjectVertexIndexDataCreator.h>

void TSimpleMesh::BeforeRenderInitAfterCreation(ID3D12Device* device, ID3D12GraphicsCommandList* list) {
	CallCreationObject(device, list);
	


	MeshComponentMaterialData MaterialCBufferData;
	MaterialCBufferData.Diffuse = { 0.8f, 0.8f, 0.8f, 1.0f };
	MaterialCBufferData.Ambient = { 0.1f, 0.1f, 0.1f, 1.0f };
	MaterialCBufferData.Emissive = { 0.0f, 0.0f, 0.0f, 1.0f };
	MaterialCBufferData.Specular = { 0.5f, 0.5f, 0.5f, 1.0f };
	MaterialCBufferData.Roughness = 0.5f;
	MaterialCBufferData.Metalness = 0.0f;
	MaterialCBufferData.SpecularPower = 32.0f;
	MaterialCBufferData.HeightScale = 0.04f;

	for (int i = 0; i < TEXTURE_TYPE_SIZE; ++i)
	{
		auto type = FD3DW::TextureType(i);
		if (m_mPathToTextures.contains(type))
		{
			m_pMaterial->SetTexture(m_mPathToTextures.at(type), type, device, list);
			MaterialCBufferData.LoadedTexture[type] = (int)GlobalTextureHeap::GetInstance()->AddTexture(m_pMaterial->GetTexture(type), device);
			MaterialCBufferData.LoadedTexture[IS_ORM_TEXTURE_FLAG_POS] = m_pMaterial->IsORMTextureType();
		}
		else
		{
			MaterialCBufferData.LoadedTexture[i] = -1;
		}
	}

	MeshComponentCreationData data;
	data.BoneBuffer = nullptr;
	data.ID = 0;
	data.IndexBuffer = m_pObjectVBV_IBV->GetIndexBufferResource();
	data.IndexBufferView = m_pObjectVBV_IBV->GetIndexBufferView();
	data.VertexBuffer = m_pObjectVBV_IBV->GetVertexBufferResource();
	data.VertexBufferView = m_pObjectVBV_IBV->GetVertexBufferView();
	data.VertexStructSize = (UINT)m_pObjectVBV_IBV->GetVertexStructSize();
	data.MaterialCBufferData = MaterialCBufferData;
	data.ObjectDescriptor = m_pObject->GetObjectParameters(0);
	
	auto meshCmp = AddComponent<MeshComponent>();
	meshCmp->SetCreationData(data);

}

void TSimpleMesh::BeforeRenderInitAfterLoad(ID3D12Device* device, ID3D12GraphicsCommandList* list) {
	CallCreationObject(device, list);

	auto meshCmp = GetComponent<MeshComponent>();
	if (!meshCmp) return;

	auto data = meshCmp->GetCreationData();


	for (int i = 0; i < TEXTURE_TYPE_SIZE; ++i)
	{
		auto type = FD3DW::TextureType(i);
		if (m_mPathToTextures.contains(type))
		{
			m_pMaterial->SetTexture(m_mPathToTextures.at(type), type, device, list);
			data.MaterialCBufferData.LoadedTexture[type] = (int)GlobalTextureHeap::GetInstance()->AddTexture(m_pMaterial->GetTexture(type), device);
			data.MaterialCBufferData.LoadedTexture[IS_ORM_TEXTURE_FLAG_POS] = m_pMaterial->IsORMTextureType();
		}
		else
		{
			data.MaterialCBufferData.LoadedTexture[i] = -1;
		}
	}

	data.BoneBuffer = nullptr;
	data.IndexBuffer = m_pObjectVBV_IBV->GetIndexBufferResource();
	data.IndexBufferView = m_pObjectVBV_IBV->GetIndexBufferView();
	data.VertexBuffer = m_pObjectVBV_IBV->GetVertexBufferResource();
	data.VertexBufferView = m_pObjectVBV_IBV->GetVertexBufferView();
	data.VertexStructSize = (UINT)m_pObjectVBV_IBV->GetVertexStructSize();
	data.ObjectDescriptor = m_pObject->GetObjectParameters(0);


	meshCmp->SetCreationData(data);
}

void TSimpleMesh::SetupTexture(FD3DW::TextureType type, std::string pathTo, ID3D12Device* device, ID3D12GraphicsCommandList* list) {
	m_pMaterial->SetTexture(pathTo, type, device, list);

	m_mPathToTextures[type] = pathTo;

	auto component = GetComponent<MeshComponent>();
	if (!component) return;

	auto matData = component->GetMaterialStruct();
	matData.LoadedTexture[type] = (int)GlobalTextureHeap::GetInstance()->AddTexture(m_pMaterial->GetTexture(type), device);
	matData.LoadedTexture[IS_ORM_TEXTURE_FLAG_POS] = m_pMaterial->IsORMTextureType();
	component->SetMaterialStruct(matData);
}

void TSimpleMesh::EraseTexture(FD3DW::TextureType type, ID3D12Device* device) {
	auto texture = m_pMaterial->GetTexture(type);
	GlobalTextureHeap::GetInstance()->RemoveTexture(texture.get(), device);
	m_pMaterial->DeleteTexture(type);

	m_mPathToTextures.erase(type);

	auto component = GetComponent<MeshComponent>();
	if (!component) return;

	auto matData = component->GetMaterialStruct();
	matData.LoadedTexture[type] = -1;
	matData.LoadedTexture[IS_ORM_TEXTURE_FLAG_POS] = m_pMaterial->IsORMTextureType();
	component->SetMaterialStruct(matData);
}

ID3D12Resource* TSimpleMesh::GetTexture(FD3DW::TextureType type) {
	return m_pMaterial->GetResourceTexture(type);
}

D3D12_GPU_DESCRIPTOR_HANDLE TSimpleMesh::GetTextureSRV(FD3DW::TextureType type) {
	return GlobalTextureHeap::GetInstance()->GetResult()->GetGPUDescriptorHandle((UINT)GlobalTextureHeap::GetInstance()->GetIndex(m_pMaterial->GetTexture(type).get()));;
}


void TSimpleMesh::CallCreationObject(ID3D12Device* device, ID3D12GraphicsCommandList* list) {
	m_pObject = CreateSimpleObject(device, list);

	m_pObjectVBV_IBV = std::make_unique<FD3DW::ObjectVertexIndexDataCreator<FD3DW::SceneVertexFrameWork>>();
	m_pObjectVBV_IBV->Create(device, list, m_pObject->GetVertices(), m_pObject->GetIndices());

	m_pMaterial = std::make_unique<FD3DW::Material>();
}
