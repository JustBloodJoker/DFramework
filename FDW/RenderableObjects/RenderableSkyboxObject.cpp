#include <RenderableObjects/RenderableSkyboxObject.h>
#include <MainRenderer/PSOManager.h>

RenderableSkyboxObject::RenderableSkyboxObject(const std::string& pathToSkybox) : BaseRenderableObject("Skybox object") {
	m_sTexturePath = pathToSkybox;
}

void RenderableSkyboxObject::Init(ID3D12Device* device, ID3D12GraphicsCommandList* list) {	
	m_pCube = std::make_unique<FD3DW::Cube>(device, list);
	m_pMatricesBuffer = FD3DW::UploadBuffer<FD3DW::MatricesConstantBufferStructureFrameWork>::CreateConstantBuffer(device, 1);

	auto cbvsrvuavsize = GetCBV_SRV_UAVDescriptorSize(device);

	m_pMaterial = std::make_unique<FD3DW::Material>();
	m_pMaterial->SetTexture(m_sTexturePath, FD3DW::TextureType::BASE, device, list);

	m_pSRVPack = FD3DW::SRV_UAVPacker::CreatePack(cbvsrvuavsize, 1, 0, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, device);
	m_pSRVPack->AddResource(m_pMaterial->GetResourceTexture(FD3DW::TextureType::BASE), D3D12_SRV_DIMENSION_TEXTURECUBE, 0, device);
}

void RenderableSkyboxObject::BeforeRender(const BeforeRenderInputData& data) {
	FD3DW::MatricesConstantBufferStructureFrameWork cmb;

	cmb.Projection = dx::XMMatrixTranspose(data.Projection);

	dx::XMMATRIX viewNoTranslation = data.View;
	viewNoTranslation.r[3] = dx::XMVectorSet(0, 0, 0, 1);
	cmb.View = dx::XMMatrixTranspose(viewNoTranslation);

	cmb.World = dx::XMMatrixTranspose(dx::XMMatrixIdentity());

	m_pMatricesBuffer->CpyData(0, cmb);
}

void RenderableSkyboxObject::DeferredRender(ID3D12GraphicsCommandList* list) {
	//do nothing here
}

void RenderableSkyboxObject::ForwardRender(ID3D12GraphicsCommandList* list) {
	PSOManager::GetInstance()->GetPSOObject(PSOType::SimpleSkyboxDefaultConfig)->Bind(list);

	list->IASetVertexBuffers(0, 1, m_pCube->GetVertexBufferView());
	list->IASetIndexBuffer(m_pCube->GetIndexBufferView());
	
	list->SetGraphicsRootConstantBufferView(0, m_pMatricesBuffer->GetGPULocation(0));

	ID3D12DescriptorHeap* heaps[] = { m_pSRVPack->GetResult()->GetDescriptorPtr() };
	list->SetDescriptorHeaps(_countof(heaps), heaps);
	list->SetGraphicsRootDescriptorTable(1, m_pSRVPack->GetResult()->GetGPUDescriptorHandle(0));

	auto cubeptr = m_pCube.get();
	list->DrawIndexedInstanced( GetIndexSize(cubeptr,0), 1, GetIndexStartPos(cubeptr, 0), GetVertexStartPos(cubeptr, 0), 0);
}

void RenderableSkyboxObject::PreDepthRender(ID3D12GraphicsCommandList* list) {
	//nothing to do
}

RenderPass RenderableSkyboxObject::GetRenderPass() const {
	return RenderPass::Forward;
}

void RenderableSkyboxObject::InitBLASBuffers(ID3D12Device5* device, ID3D12GraphicsCommandList4* list) {
	//nothing to do
}
