#include "pch.h"

#include <Entity/RenderObject/TSimpleMesh.h>
#include <Renderer/GlobalTextureHeap.h>
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
		}
		else
		{
			MaterialCBufferData.LoadedTexture[i] = -1;
		}
	}

	MeshComponentCreationData data;
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
		}
		else
		{
			data.MaterialCBufferData.LoadedTexture[i] = -1;
		}
	}

	data.IndexBuffer = m_pObjectVBV_IBV->GetIndexBufferResource();
	data.IndexBufferView = m_pObjectVBV_IBV->GetIndexBufferView();
	data.VertexBuffer = m_pObjectVBV_IBV->GetVertexBufferResource();
	data.VertexBufferView = m_pObjectVBV_IBV->GetVertexBufferView();
	data.VertexStructSize = (UINT)m_pObjectVBV_IBV->GetVertexStructSize();
	data.ObjectDescriptor = m_pObject->GetObjectParameters(0);


	meshCmp->SetCreationData(data);
}

void TSimpleMesh::SetDiffuse(dx::XMFLOAT4 data) {
	auto* component = GetComponent<MeshComponent>();
	if (!component) return;

	auto material = component->GetMaterialStruct();
	material.Diffuse = data;
	component->SetMaterialStruct(material);
}


void TSimpleMesh::CallCreationObject(ID3D12Device* device, ID3D12GraphicsCommandList* list) {
	m_pObject = CreateSimpleObject(device, list);

	m_pObjectVBV_IBV = std::make_unique<FD3DW::ObjectVertexIndexDataCreator<FD3DW::VertexFrameWork>>();
	m_pObjectVBV_IBV->Create(device, list, m_pObject->GetVertices(), m_pObject->GetIndices());

	m_pMaterial = std::make_unique<FD3DW::Material>();
}
