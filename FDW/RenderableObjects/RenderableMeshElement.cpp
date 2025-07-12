#include <RenderableObjects/RenderableMeshElement.h>

int RenderableMeshElement::__MeshElementsCreatedCount = 0;

RenderableMeshElement::RenderableMeshElement(const RenderableMeshElementData& data) : BaseRenderableObject("SubMesh_ID_" + std::to_string(__MeshElementsCreatedCount)) {
	m_xData = data;

	++__MeshElementsCreatedCount;
}

void RenderableMeshElement::Init(ID3D12Device* device, ID3D12GraphicsCommandList* list) {
	m_pMatricesBuffer = FD3DW::UploadBuffer<FD3DW::MatricesConstantBufferStructureFrameWork>::CreateConstantBuffer(device, 1);
}

void RenderableMeshElement::BeforeRender(const BeforeRenderInputData& data) {
	FD3DW::MatricesConstantBufferStructureFrameWork cmb;
	cmb.Projection = dx::XMMatrixTranspose(data.Projection);
	cmb.View = dx::XMMatrixTranspose(data.View);
	cmb.World = dx::XMMatrixTranspose(m_xWorldMatrix * data.AdditionalWorld);
	m_pMatricesBuffer->CpyData(0, cmb);
}

void RenderableMeshElement::Render(ID3D12GraphicsCommandList* list) {
	list->SetGraphicsRootConstantBufferView(0, m_pMatricesBuffer->GetGPULocation(0));

	ID3D12DescriptorHeap* heaps[] = { m_xData.SRVPack->GetResult()->GetDescriptorPtr() };
	list->SetDescriptorHeaps(_countof(heaps), heaps);
	list->SetGraphicsRootDescriptorTable(1, m_xData.SRVPack->GetResult()->GetGPUDescriptorHandle(0));

	list->DrawIndexedInstanced(m_xData.ObjectDescriptor.IndicesCount, 1, m_xData.ObjectDescriptor.IndicesOffset, m_xData.ObjectDescriptor.VerticesOffset, 0);
}
