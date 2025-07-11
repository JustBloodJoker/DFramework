#include <RenderableObjects/RenderableMesh.h>
#include <MainRenderer/PSOManager.h>

RenderableMesh::RenderableMesh(std::unique_ptr<FD3DW::Scene> scene) : BaseRenderableObject( scene->GetPath() ) {
	m_pScene = std::move(scene);

}

void RenderableMesh::Init(ID3D12Device* device, ID3D12GraphicsCommandList* list) {
	auto cbvsrvuavsize = GetCBV_SRV_UAVDescriptorSize(device);

	for (size_t ind = 0; ind < m_pScene->GetMaterialSize(); ind++)
	{
		m_vSRVPacks.push_back(FD3DW::SRVPacker::CreatePack(cbvsrvuavsize, 1, 0, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, device));
		if (m_pScene->GetMaterialMananger()->GetMaterial(ind)->IsHaveTexture(TextureType::BASE))
		{
			m_vSRVPacks.back()->PushResource(m_pScene->GetMaterialMananger()->GetMaterial(ind)->GetResourceTexture(TextureType::BASE), D3D12_SRV_DIMENSION_TEXTURE2D, device);
		}
	}

	m_pSamplerPack = FD3DW::SamplerPacker::CreatePack(cbvsrvuavsize, 1u, 0, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, device);
	m_pSamplerPack->PushDefaultSampler(device);
	
	if (auto size = m_pScene->GetBonesCount() * sizeof(dx::XMMATRIX)) {
		m_pStructureBufferBones = FD3DW::FResource::CreateSimpleStructuredBuffer(device, size);
	}
	
	for (auto ind = 0; ind < m_pScene->GetObjectBuffersCount(); ind++)
	{
		RenderableMeshElementData data;
		data.ObjectDescriptor = m_pScene->GetObjectParameters(ind);
		data.SamplerPack = m_pSamplerPack.get();
		data.SRVPack = m_vSRVPacks[GetMaterialIndex(m_pScene.get(), ind)].get();

		auto elem = std::make_unique<RenderableMeshElement>(data);
		elem->Init(device, list);

		m_vRenderableElements.push_back( std::move(elem) );
	}

}

void RenderableMesh::BeforeRender(const BeforeRenderInputData& data) {
	AnimationTickUpdate(data);

	auto cpyData = data;
	cpyData.AdditionalWorld = m_xWorldMatrix * data.AdditionalWorld;

	for (auto& elem : m_vRenderableElements) {
		elem->BeforeRender(cpyData);
	}
}

void RenderableMesh::Render(ID3D12GraphicsCommandList* list) {
	
	if (m_pStructureBufferBones) {
		PSOManager::GetInstance()->GetPSOObject(PSOType::DefferedFirstPassAnimatedMeshesDefaultConfig)->Bind(list);
		list->SetGraphicsRootShaderResourceView(3, m_pStructureBufferBones->GetResource()->GetGPUVirtualAddress());
	}
	else {
		PSOManager::GetInstance()->GetPSOObject(PSOType::DefferedFirstPassSimpleMeshesDefaultConfig)->Bind(list);
	}


	list->IASetVertexBuffers(0, 1, m_pScene->GetVertexBufferView());
	list->IASetIndexBuffer(m_pScene->GetIndexBufferView());

	for (auto& elem : m_vRenderableElements) {
		elem->Render(list);
	}
}

std::vector<std::string> RenderableMesh::GetAnimations() {
	return m_pScene->GetAnimations();
}

void RenderableMesh::PlayAnimation(std::string animName) {
	m_sCurrentAnimation = animName;
}

void RenderableMesh::StopAnimation() {
	if (m_sCurrentAnimation == "") return;

	m_sCurrentAnimation = "";
	m_bNeedResetBonesBuffer = true;
}

void RenderableMesh::FreezeAnimation(bool isFreezed) {
	m_bNeedFreezeBonesBuffer = isFreezed;
}

std::vector<RenderableMeshElement*> RenderableMesh::GetRenderableElements() {
	std::vector<RenderableMeshElement*> ret;
	ret.reserve(m_vRenderableElements.size());

	for (const auto& elem : m_vRenderableElements) {
		ret.push_back(elem.get());
	}

	return ret;
}

void RenderableMesh::AnimationTickUpdate(const BeforeRenderInputData& data) {
	if (!m_pStructureBufferBones || m_bNeedFreezeBonesBuffer) return;

	std::vector<dx::XMMATRIX> dataVec;
	if (!m_sCurrentAnimation.empty()) {
		dataVec = m_pScene->PlayAnimation(data.Time, m_sCurrentAnimation);
	}
	else if(m_bNeedResetBonesBuffer) {
		dataVec.resize( m_pScene->GetBonesCount() );
	}

	if (!dataVec.empty()) {
		m_pStructureBufferBones->UploadData(data.Device, data.CommandList, dataVec.data(), false, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}

}
