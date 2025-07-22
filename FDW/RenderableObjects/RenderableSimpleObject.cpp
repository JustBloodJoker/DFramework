#include <RenderableObjects/RenderableSimpleObject.h>
#include <MainRenderer/PSOManager.h>


RenderableSimpleObject::RenderableSimpleObject(std::unique_ptr<FD3DW::SimpleObject<FD3DW::SceneVertexFrameWork>> object) :BaseRenderableObject("Simple object") {
	m_pObject = std::move(object);
}

void RenderableSimpleObject::Init(ID3D12Device* device, ID3D12GraphicsCommandList* list) {
	m_pMatricesBuffer = FD3DW::UploadBuffer<MeshMatricesStructure>::CreateConstantBuffer(device, 1);

	m_pMaterialBuffer = FD3DW::UploadBuffer<MeshMaterialStructure>::CreateConstantBuffer(device, 1);

	auto cbvsrvuavsize = GetCBV_SRV_UAVDescriptorSize(device);

	m_pMaterial = std::make_unique<FD3DW::Material>();
	
	m_pSRVPack = FD3DW::SRVPacker::CreatePack(cbvsrvuavsize, FD3DW::TextureType::SIZE, 0, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, device);
	
	m_xMaterialCBufferData.Diffuse = { 0.8f, 0.8f, 0.8f, 1.0f };
	m_xMaterialCBufferData.Ambient = { 0.1f, 0.1f, 0.1f, 1.0f };
	m_xMaterialCBufferData.Emissive = { 0.0f, 0.0f, 0.0f, 1.0f };
	m_xMaterialCBufferData.Specular = { 0.5f, 0.5f, 0.5f, 1.0f };
	m_xMaterialCBufferData.Roughness = 0.5f;
	m_xMaterialCBufferData.Metalness = 0.0f;
	m_xMaterialCBufferData.SpecularPower = 32.0f;
	m_xMaterialCBufferData.HeightScale = 0.04f;

	for (int i = 0; i < FD3DW::TextureType::SIZE; ++i) {
		m_xMaterialCBufferData.LoadedTexture[i] = false;
		m_pSRVPack->AddNullResource(i, device);

	}

	NeedUpdateMaterials();
}

void RenderableSimpleObject::BeforeRender(const BeforeRenderInputData& data) {
	MeshMatricesStructure cmb;
	cmb.Projection = dx::XMMatrixTranspose(data.Projection);
	cmb.View = dx::XMMatrixTranspose(data.View);
	cmb.World = dx::XMMatrixTranspose(m_xWorldMatrix * data.AdditionalWorld);
	cmb.IsActiveAnimation = false;
	cmb.CameraPosition = data.CameraPosition;

	m_pMatricesBuffer->CpyData(0, cmb);

	BeforeRenderMaterialsUpdate();
}

void RenderableSimpleObject::DeferredRender(ID3D12GraphicsCommandList* list) {
	list->SetGraphicsRootShaderResourceView(ANIMATIONS_CONSTANT_BUFFER_IN_ROOT_SIG, GetEmptyStructuredBufferGPUVirtualAddress());

	list->IASetVertexBuffers(0, 1, m_pObject->GetVertexBufferView());
	list->IASetIndexBuffer(m_pObject->GetIndexBufferView());

	list->SetGraphicsRootConstantBufferView(CONSTANT_BUFFER_MATRICES_POSITION_IN_ROOT_SIG, m_pMatricesBuffer->GetGPULocation(0));

	list->SetGraphicsRootConstantBufferView(CONSTANT_BUFFER_MATERIALS_POSITION_IN_ROOT_SIG, m_pMaterialBuffer->GetGPULocation(0));

	ID3D12DescriptorHeap* heaps[] = { m_pSRVPack->GetResult()->GetDescriptorPtr() };
	list->SetDescriptorHeaps(_countof(heaps), heaps);
	list->SetGraphicsRootDescriptorTable(TEXTURE_START_POSITION_IN_ROOT_SIG, m_pSRVPack->GetResult()->GetGPUDescriptorHandle(0));

	auto params = m_pObject->GetObjectParameters(0);
	list->DrawIndexedInstanced(params.IndicesCount, 1, params.IndicesOffset, params.VerticesOffset, 0);

}

void RenderableSimpleObject::ForwardRender(ID3D12GraphicsCommandList* list) {
	//TODO
}

RenderPass RenderableSimpleObject::GetRenderPass() const {
	return RenderPass::Deferred;
}

void RenderableSimpleObject::SetupTexture(FD3DW::TextureType type, std::string pathTo, ID3D12Device* device, ID3D12GraphicsCommandList* list) {
	m_pMaterial->SetTexture(pathTo, type, device, list);
	m_pSRVPack->AddResource(m_pMaterial->GetResourceTexture(type), D3D12_SRV_DIMENSION_TEXTURE2D, type, device);
	
	m_xMaterialCBufferData.LoadedTexture[type] = true;
	m_xMaterialCBufferData.LoadedTexture[FD3DW::TextureType::SIZE] = m_pMaterial->IsRoughnessAndMetalnessInOneTexture();
	NeedUpdateMaterials();
}

void RenderableSimpleObject::EraseTexture(FD3DW::TextureType type, ID3D12Device* device) {
	m_pMaterial->DeleteTexture(type);
	m_pSRVPack->AddNullResource(type, device);
	
	m_xMaterialCBufferData.LoadedTexture[type] = false;
	m_xMaterialCBufferData.LoadedTexture[FD3DW::TextureType::SIZE] = m_pMaterial->IsRoughnessAndMetalnessInOneTexture();
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
		m_pMaterialBuffer->CpyData(0, m_xMaterialCBufferData);
		m_bIsMaterialDataChanged = false;
	}

}

void RenderableSimpleObject::NeedUpdateMaterials() {
	m_bIsMaterialDataChanged = true;
}
