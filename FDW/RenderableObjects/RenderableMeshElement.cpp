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
	m_xData.MaterialCBufferData.LoadedTexture = data.MaterialCBufferData.LoadedTexture;
	m_xData.ObjectDescriptor = data.ObjectDescriptor;
}

void RenderableMeshElement::Init(ID3D12Device* device, ID3D12GraphicsCommandList* list) {
	m_pMatricesBuffer = FD3DW::UploadBuffer<MeshMatricesStructure>::CreateConstantBuffer(device, 1);

	m_pMaterialBuffer = FD3DW::UploadBuffer<MeshMaterialStructure>::CreateConstantBuffer(device, 1);
	m_bIsMaterialDataChanged = true;

	UpdateWorldMatrix();
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
	//nothing to do
}

void RenderableMeshElement::ForwardRender(ID3D12GraphicsCommandList* list) {
	//nothing to do
}

RenderPass RenderableMeshElement::GetRenderPass() const {
	return RenderPass::Deferred;
}

bool RenderableMeshElement::IsCanBeIndirectExecuted() {
	return true;
}

std::vector<IndirectMeshRenderableData> RenderableMeshElement::GetDataToExecute() {
	IndirectMeshRenderableData data;
	data.CBMaterials = m_pMaterialBuffer->GetGPULocation(0);
	data.CBMatrices = m_pMatricesBuffer->GetGPULocation(0);
	data.DrawArguments.IndexCountPerInstance = m_xData.ObjectDescriptor.IndicesCount;
	data.DrawArguments.InstanceCount = 1u;
	data.DrawArguments.StartIndexLocation = m_xData.ObjectDescriptor.IndicesOffset;
	data.DrawArguments.BaseVertexLocation = (INT)m_xData.ObjectDescriptor.VerticesOffset;
	data.DrawArguments.StartInstanceLocation = 0u;
	return { data };
}


void RenderableMeshElement::BeforeRenderCBMaterial() {
	if (m_bIsMaterialDataChanged) {
		m_pMaterialBuffer->CpyData(0, m_xData.MaterialCBufferData);
		m_bIsMaterialDataChanged = false;
	}
}
