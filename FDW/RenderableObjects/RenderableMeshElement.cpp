#include <RenderableObjects/RenderableMeshElement.h>

int RenderableMeshElement::__MeshElementsCreatedCount = 0;

RenderableMeshElement::RenderableMeshElement() {
	++__MeshElementsCreatedCount;
}

RenderableMeshElement::RenderableMeshElement(const RenderableMeshElementData& data) : BaseRenderableObject("SubMesh_ID_" + std::to_string(__MeshElementsCreatedCount)) {
	m_xData = data;

	++__MeshElementsCreatedCount;
}

void RenderableMeshElement::SetRenderableMeshElementData(const RenderableMeshElementData& data) {
	m_xData.SRVPack = data.SRVPack;
	m_xData.MaterialCBufferData.LoadedTexture = data.MaterialCBufferData.LoadedTexture;
	m_xData.ObjectDescriptor = data.ObjectDescriptor;
}

void RenderableMeshElement::Init(ID3D12Device* device, ID3D12GraphicsCommandList* list) {
	m_pMatricesBuffer = FD3DW::UploadBuffer<MeshMatricesStructure>::CreateConstantBuffer(device, 1);

	m_pMaterialBuffer = FD3DW::UploadBuffer<MeshMaterialStructure>::CreateConstantBuffer(device, 1);
	m_bIsMaterialDataChanged = true;
}

void RenderableMeshElement::BeforeRender(const BeforeRenderInputData& data) {
	MeshMatricesStructure cmb;
	cmb.Projection = dx::XMMatrixTranspose(data.Projection);
	cmb.View = dx::XMMatrixTranspose(data.View);
	cmb.World = dx::XMMatrixTranspose(m_xWorldMatrix * data.AdditionalWorld);
	cmb.IsActiveAnimation = m_bIsAnimationPlaying;
	cmb.CameraPosition = data.CameraPosition;

	m_pMatricesBuffer->CpyData(0, cmb);

	BeforeRenderCBMaterial();
}

void RenderableMeshElement::SetAnimationPlaying(bool isP) {
	m_bIsAnimationPlaying = isP;
}

void RenderableMeshElement::DeferredRender(ID3D12GraphicsCommandList* list) {
	list->SetGraphicsRootConstantBufferView(CONSTANT_BUFFER_MATRICES_POSITION_IN_ROOT_SIG, m_pMatricesBuffer->GetGPULocation(0));

	list->SetGraphicsRootConstantBufferView(CONSTANT_BUFFER_MATERIALS_POSITION_IN_ROOT_SIG, m_pMaterialBuffer->GetGPULocation(0));

	ID3D12DescriptorHeap* heaps[] = { m_xData.SRVPack->GetResult()->GetDescriptorPtr() };
	list->SetDescriptorHeaps(_countof(heaps), heaps);
	
	list->SetGraphicsRootDescriptorTable(TEXTURE_START_POSITION_IN_ROOT_SIG, m_xData.SRVPack->GetResult()->GetGPUDescriptorHandle(0));

	list->DrawIndexedInstanced(m_xData.ObjectDescriptor.IndicesCount, 1, m_xData.ObjectDescriptor.IndicesOffset, m_xData.ObjectDescriptor.VerticesOffset, 0);
}

void RenderableMeshElement::ForwardRender(ID3D12GraphicsCommandList* list) {
	//OPACITY FILTER NOT IMPL
}

RenderPass RenderableMeshElement::GetRenderPass() const {
	return RenderPass::Deferred;
}

void RenderableMeshElement::BeforeRenderCBMaterial() {
	if (m_bIsMaterialDataChanged) {
		m_pMaterialBuffer->CpyData(0, m_xData.MaterialCBufferData);
		m_bIsMaterialDataChanged = false;
	}
}
