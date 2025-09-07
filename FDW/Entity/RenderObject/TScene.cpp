#include <Entity/RenderObject/TScene.h>
#include <MainRenderer/GlobalRenderThreadManager.h>
#include <MainRenderer/GlobalTextureHeap.h>
#include <World/World.h>



TScene::TScene(std::string path) {
	m_sPath = path;
	m_sName = path;
}


bool TScene::IsHaveAnimation() {
	return GetAnimationComponent() != nullptr;
}

bool TScene::IsFreeze() {
	auto animCmp = GetAnimationComponent();
	if (!animCmp) return false;

	return animCmp->IsFreeze();
}
void TScene::Freeze(bool b) {
	auto animCmp = GetAnimationComponent();
	if (!animCmp) return;

	animCmp->Freeze(b);
}

std::vector<std::string> TScene::GetAnimations() {
	auto animCmp = GetAnimationComponent();
	if (!animCmp) return {};

	return animCmp->GetAnimations();
}
void TScene::Play(std::string animName) {
	auto animCmp = GetAnimationComponent();
	if (!animCmp) return;

	animCmp->Play(animName);
}
void TScene::Stop() {
	auto animCmp = GetAnimationComponent();
	if (!animCmp) return;

	animCmp->Stop();
}
bool TScene::IsPlaying() {
	auto animCmp = GetAnimationComponent();
	if (!animCmp) return false;

	return animCmp->IsPlaying();
}

void TScene::BeforeRenderInitAfterCreation(ID3D12Device* device, ID3D12GraphicsCommandList* list) {
	CallCreationScene(device, list);

	AnimationComponent* animCmp = nullptr;
	if ( !m_pScene->GetAnimations().empty() ) {
		animCmp = AddComponent<AnimationComponent>();
		animCmp->SetScene(device, m_pScene.get());
	}

	std::vector<MeshComponentMaterialData> meshMaterialStructures;

	for (size_t ind = 0; ind < m_pScene->GetMaterialSize(); ind++)
	{
		MeshComponentMaterialData cbData;

		auto* mat = m_pScene->GetMaterialMananger()->GetMaterial(ind);
		cbData = mat->GetMaterialDesc();
		for (int i = 0; i < TEXTURE_TYPE_SIZE; ++i) {
			if (mat->IsHaveTexture(FD3DW::TextureType(i)))
			{
				cbData.LoadedTexture[i] = (int)GlobalTextureHeap::GetInstance()->AddTexture(mat->GetTexture(FD3DW::TextureType(i)), device);
			}
			else {
				cbData.LoadedTexture[i] = -1;
			}
		}
		cbData.LoadedTexture[IS_ORM_TEXTURE_FLAG_POS] = mat->IsORMTextureType();

		meshMaterialStructures.push_back(cbData);
	}

	for (auto ind = 0; ind < m_pScene->GetObjectBuffersCount(); ind++)
	{
		MeshComponentCreationData data;
		data.ObjectDescriptor = m_pScene->GetObjectParameters(ind);

		auto matIndex = (UINT)m_pScene->GetObjectParameters(ind).MaterialIndex;

		data.MaterialCBufferData = meshMaterialStructures[matIndex];
		data.ID = ind;
		data.BoneBuffer = animCmp ? animCmp->GetResource() : nullptr;
		if(animCmp) data.IsBoneActive = animCmp->IsBonesActive();
		data.IndexBuffer = m_pScene->GetIndexBuffer();
		data.VertexBuffer = m_pScene->GetVertexBuffer();
		data.VertexStructSize = (UINT)m_pScene->GetVertexStructSize();
		data.IndexBufferView = m_pScene->GetIndexBufferView();
		data.VertexBufferView = m_pScene->GetVertexBufferView();
		
		auto cmp = AddComponent<MeshComponent>();
		cmp->SetCreationData(data);
	}
	

}

void TScene::BeforeRenderInitAfterLoad(ID3D12Device* device, ID3D12GraphicsCommandList* list) {
	CallCreationScene(device, list);

	AnimationComponent* animCmp = GetComponent<AnimationComponent>();
	if (!m_pScene->GetAnimations().empty() && animCmp) {
		animCmp->SetScene(device, m_pScene.get());
	}

	std::vector<MeshComponentMaterialData> meshMaterialStructures;

	for (size_t ind = 0; ind < m_pScene->GetMaterialSize(); ind++)
	{
		MeshComponentMaterialData cbData;

		auto* mat = m_pScene->GetMaterialMananger()->GetMaterial(ind);
		cbData = mat->GetMaterialDesc();
		for (int i = 0; i < TEXTURE_TYPE_SIZE; ++i) {
			if (mat->IsHaveTexture(FD3DW::TextureType(i)))
			{
				cbData.LoadedTexture[i] = (int)GlobalTextureHeap::GetInstance()->AddTexture(mat->GetTexture(FD3DW::TextureType(i)), device);
			}
			else {
				cbData.LoadedTexture[i] = -1;
			}
		}
		cbData.LoadedTexture[IS_ORM_TEXTURE_FLAG_POS] = mat->IsORMTextureType();

		meshMaterialStructures.push_back(cbData);
	}

	auto meshComponents = GetComponents<MeshComponent>();
	for (auto& meshCmp : meshComponents) {
		auto data = meshCmp->GetCreationData();

		data.BoneBuffer = animCmp ? animCmp->GetResource() : nullptr;
		data.ObjectDescriptor = m_pScene->GetObjectParameters(data.ID);
		auto matIndex = (UINT)data.ObjectDescriptor.MaterialIndex;
		data.MaterialCBufferData = meshMaterialStructures[matIndex];
		data.IndexBuffer = m_pScene->GetIndexBuffer();
		data.VertexBuffer = m_pScene->GetVertexBuffer();
		data.VertexStructSize = (UINT)m_pScene->GetVertexStructSize();
		data.IndexBufferView = m_pScene->GetIndexBufferView();
		data.VertexBufferView = m_pScene->GetVertexBufferView();

		meshCmp->SetCreationData(data);
	}
}

void TScene::CallCreationScene(ID3D12Device* device, ID3D12GraphicsCommandList* list) {
	m_pScene = std::make_unique<FD3DW::Scene>(m_sPath, device, list, true);
}

AnimationComponent* TScene::GetAnimationComponent() {
	return GetComponent<AnimationComponent>();
}
