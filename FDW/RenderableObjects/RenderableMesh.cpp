#include <RenderableObjects/RenderableMesh.h>

RenderableMesh::RenderableMesh(std::string path) : BaseRenderableObject(path) {
	m_sPath = path;
}

void RenderableMesh::Init(ID3D12Device* device, ID3D12GraphicsCommandList* list) {
	m_pScene = std::make_unique<FD3DW::Scene>(m_sPath, device, list, true);

	auto cbvsrvuavsize = GetCBV_SRV_UAVDescriptorSize(device);

	std::vector<MeshMaterialStructure> meshMaterialStructures;

	for (size_t ind = 0; ind < m_pScene->GetMaterialSize(); ind++)
	{
		m_vSRVPacks.push_back(FD3DW::SRV_UAVPacker::CreatePack(cbvsrvuavsize, TEXTURE_TYPE_SIZE, 0, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, device));
		MeshMaterialStructure cbData;
		
		auto* mat = m_pScene->GetMaterialMananger()->GetMaterial(ind);
		cbData = mat->GetMaterialDesc();
		for (int i = 0; i < TEXTURE_TYPE_SIZE; ++i) {
			if ( cbData.LoadedTexture[i] = mat->IsHaveTexture(FD3DW::TextureType(i) ) )
			{
				m_vSRVPacks.back()->AddResource(mat->GetResourceTexture(FD3DW::TextureType(i)), D3D12_SRV_DIMENSION_TEXTURE2D, i, device);
			}
			else {

				m_vSRVPacks.back()->AddNullResource(i, device);
			}
		}
		cbData.LoadedTexture[IS_ORM_TEXTURE_FLAG_POS] = mat->IsORMTextureType();

		meshMaterialStructures.push_back(cbData);
	}
	
	if (m_pScene->GetBonesCount()) {
		m_pStructureBufferBones = FD3DW::StructuredBuffer::CreateStructuredBuffer<dx::XMMATRIX>(device, UINT(m_pScene->GetBonesCount()), false);
	}
	
	for (auto ind = 0; ind < m_pScene->GetObjectBuffersCount(); ind++)
	{
		RenderableMeshElementData data;
		data.ObjectDescriptor = m_pScene->GetObjectParameters(ind);
		
		auto matIndex = GetMaterialIndex(m_pScene.get(), ind);
		data.SRVPack = m_vSRVPacks[matIndex].get();
		data.MaterialCBufferData = meshMaterialStructures[matIndex];
		data.ID = ind;
		if (m_vRenderableElements.size() <= ind) 
		{
			auto elem = std::make_unique<RenderableMeshElement>(data);
			elem->Init(device, list);
			m_vRenderableElements.push_back(std::move(elem));
		}
		else 
		{
			m_vRenderableElements[ind]->SetRenderableMeshElementData(data);
			m_vRenderableElements[ind]->Init(device, list);
		}
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

void RenderableMesh::DeferredRender(ID3D12GraphicsCommandList* list) {
	RenderObjectsInPass(RenderPass::Deferred, list);
}

void RenderableMesh::ForwardRender(ID3D12GraphicsCommandList* list) {
	RenderObjectsInPass(RenderPass::Forward, list);
}

RenderPass RenderableMesh::GetRenderPass() const {
	return RenderPass::DeferredAndForward;
}

std::vector<std::string> RenderableMesh::GetAnimations() {
	return m_pScene->GetAnimations();
}

void RenderableMesh::PlayAnimation(std::string animName) {
	if (m_sCurrentAnimation == animName) return;
	m_sCurrentAnimation = animName;

	m_fAnimationTime = 0.f;

	for (auto& elem : m_vRenderableElements) {
		elem->SetAnimationPlaying(m_pStructureBufferBones!=nullptr && true);
	}
}

void RenderableMesh::StopAnimation() {
	if (m_sCurrentAnimation == "") return;

	m_sCurrentAnimation = "";
	m_bNeedResetBonesBuffer = true;

	for (auto& elem : m_vRenderableElements) {
		elem->SetAnimationPlaying(false);
	}
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

void RenderableMesh::InitBLASBuffers(ID3D12Device5* device, ID3D12GraphicsCommandList4* list)
{
	auto buffers = FD3DW::CreateBLASForObject(device, list, m_pScene.get(), BASE_RENDERABLE_OBJECTS_BLAS_HIT_GROUP_INDEX, true);

	auto idx = 0;
	for (auto idx = 0; idx < buffers.size(); ++idx) {
		for (const auto& element : m_vRenderableElements) {
			if (idx != element->ElementID()) continue;

			element->SetBLASBuffer(buffers[idx]);
		}
	}

}

std::vector<std::pair<FD3DW::AccelerationStructureBuffers, dx::XMMATRIX>> RenderableMesh::GetBLASInstances()
{
	std::vector<std::pair<FD3DW::AccelerationStructureBuffers, dx::XMMATRIX>> ret;
	for (const auto& element : m_vRenderableElements) {
		auto getInstances = element->GetBLASInstances();
		ret.insert(ret.end(), getInstances.begin(), getInstances.end());
	}
	return ret;
}

bool RenderableMesh::IsNeedUpdateTLAS() {
	for (const auto& element : m_vRenderableElements) {
		if (element->IsNeedUpdateTLAS()) return true;
	}
	return false;
}

void RenderableMesh::RenderObjectsInPass(RenderPass pass, ID3D12GraphicsCommandList* list) {
	if (pass==RenderPass::Forward) return; //forward not impl
	
	auto gpuStructureBufferBonesAdress = m_pStructureBufferBones ? m_pStructureBufferBones->GetResource()->GetGPUVirtualAddress() : GetEmptyStructuredBufferGPUVirtualAddress();
	list->SetGraphicsRootShaderResourceView(ANIMATIONS_CONSTANT_BUFFER_IN_ROOT_SIG, gpuStructureBufferBonesAdress);

	list->IASetVertexBuffers(0, 1, m_pScene->GetVertexBufferView());
	list->IASetIndexBuffer(m_pScene->GetIndexBufferView());

	for (const auto& elem : m_vRenderableElements) {
		if (elem->IsCanRenderInPass(pass)) {
			elem->IsCanRenderInPass(RenderPass::Deferred) ? elem->DeferredRender(list) : elem->ForwardRender(list);
		}
	}
}

void RenderableMesh::AnimationTickUpdate(const BeforeRenderInputData& data) {
	if (!m_pStructureBufferBones || m_bNeedFreezeBonesBuffer) return;

	std::vector<dx::XMMATRIX> dataVec;
	if (!m_sCurrentAnimation.empty()) {
		dataVec = m_pScene->PlayAnimation(m_fAnimationTime, m_sCurrentAnimation);
		m_fAnimationTime += data.DT;
	}
	else if(m_bNeedResetBonesBuffer) {
		dataVec.resize( m_pScene->GetBonesCount() );
		m_bNeedResetBonesBuffer = false;
	}

	if (!dataVec.empty()) {
		m_pStructureBufferBones->UploadData(data.Device, data.CommandList, dataVec.data(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}

}
