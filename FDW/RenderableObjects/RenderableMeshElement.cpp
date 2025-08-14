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

void RenderableMeshElement::BeforeDelete() {
	RenderableObjectsMatricesBuffer::GetInstance()->RemoveDataFromIndex(m_uMatricesIdxInBuffer);
	RenderableObjectsMaterialsBuffer::GetInstance()->RemoveDataFromIndex(m_uMaterialIdxInBuffer);
}

void RenderableMeshElement::Init(ID3D12Device* device, ID3D12GraphicsCommandList* list) {
	m_pBufferIndicesBuffer = FD3DW::UploadBuffer<MeshBufferIndices>::CreateConstantBuffer(device, 1);
	
	m_uMatricesIdxInBuffer = RenderableObjectsMatricesBuffer::GetInstance()->GenerateIndexForBuffer();
	MeshMatricesStructure cmb;
	RenderableObjectsMatricesBuffer::GetInstance()->UploadDataToIndex(m_uMatricesIdxInBuffer, cmb);
	
	m_uMaterialIdxInBuffer = RenderableObjectsMaterialsBuffer::GetInstance()->GenerateIndexForBuffer();
	RenderableObjectsMaterialsBuffer::GetInstance()->UploadDataToIndex(m_uMaterialIdxInBuffer, m_xData.MaterialCBufferData);

	MeshBufferIndices data;
	data.MatricesIndex = m_uMatricesIdxInBuffer;
	data.MaterialIndex = m_uMaterialIdxInBuffer;
	
	m_pBufferIndicesBuffer->CpyData(0, data);

	UpdateWorldMatrix();
}

void RenderableMeshElement::BeforeRender(const BeforeRenderInputData& data) {
	MeshMatricesStructure cmb;
	cmb.Projection = dx::XMMatrixTranspose(data.Projection);
	cmb.View = dx::XMMatrixTranspose(data.View);
	cmb.World = dx::XMMatrixTranspose(m_xWorldMatrix * data.AdditionalWorld);
	cmb.StartIndexInBoneBuffer = m_iStartIndexInBoneBuffer;
	cmb.CameraPosition = data.CameraPosition;
	RenderableObjectsMatricesBuffer::GetInstance()->UploadDataToIndex(m_uMatricesIdxInBuffer, cmb);

	BeforeRenderCBMaterial();
}

size_t RenderableMeshElement::ElementID()
{
	return m_xData.ID;
}

void RenderableMeshElement::InitBLASBuffers(ID3D12Device5* device, ID3D12GraphicsCommandList4* list) {
	//nothing to do
}

void RenderableMeshElement::SetBLASBuffer(const FD3DW::AccelerationStructureBuffers& buffer) {
	m_vBLASBuffers.clear();
	m_vBLASBuffers.push_back(buffer);
}

void RenderableMeshElement::DeferredRender(ID3D12GraphicsCommandList* list) {
	ID3D12DescriptorHeap* heaps[] = { m_xData.SRVPack->GetResult()->GetDescriptorPtr() };
	list->SetDescriptorHeaps(_countof(heaps), heaps);
	
	list->SetGraphicsRootDescriptorTable(TEXTURE_START_POSITION_IN_ROOT_SIG, m_xData.SRVPack->GetResult()->GetGPUDescriptorHandle(0));
	list->SetGraphicsRootConstantBufferView(INDICES_CONSTANT_BUFFER_IN_ROOT_SIG, m_pBufferIndicesBuffer->GetGPULocation(0));

	list->DrawIndexedInstanced(m_xData.ObjectDescriptor.IndicesCount, 1, m_xData.ObjectDescriptor.IndicesOffset, m_xData.ObjectDescriptor.VerticesOffset, 0);
}

void RenderableMeshElement::ForwardRender(ID3D12GraphicsCommandList* list) {
	//OPACITY FILTER NOT IMPL
}

RenderPass RenderableMeshElement::GetRenderPass() const {
	return RenderPass::Deferred;
}

void RenderableMeshElement::SetBonesIndex(int index) {
	m_iStartIndexInBoneBuffer = index;
}

void RenderableMeshElement::BeforeRenderCBMaterial() {
	if (m_bIsMaterialDataChanged) {
		RenderableObjectsMaterialsBuffer::GetInstance()->UploadDataToIndex(m_uMaterialIdxInBuffer, m_xData.MaterialCBufferData);
		m_bIsMaterialDataChanged = false;
	}
}
