#include <World/World.h>
#include <MainRenderer/MainRenderer.h>


void World::SetMainRenderer(MainRenderer* mr) {
	m_pRender = mr;
	AfterSetMainRenderer();
}

MainRenderer* World::GetMainRenderer() {
	return m_pRender;
}

void World::AddNotifyToPull(NRenderSystemNotifyType type) {
	if (std::find(m_vRenderSystemNotifies.begin(), m_vRenderSystemNotifies.end(), type) != m_vRenderSystemNotifies.end()) return;

	m_vRenderSystemNotifies.push_back(type);
}

std::vector<NRenderSystemNotifyType> World::GetRenderSystemNotifies() const {
	return m_vRenderSystemNotifies;
}

void World::ClearNotifies() {
	m_vRenderSystemNotifies.clear();
}

ComponentHolder* World::CreateEmptyEntity() {
	return CreateEntity<ComponentHolder>();
}

TDefaultCamera* World::CreateDefaultCamera() {
	return CreateEntity<TDefaultCamera>();
}

TDirectionalLight* World::CreateDirectionalLight() {
	return CreateEntity<TDirectionalLight>();
}

TPointLight* World::CreatePointLight() {
	return CreateEntity<TPointLight>();
}

TSpotLight* World::CreateSpotLight() {
	return CreateEntity<TSpotLight>();
}

TRectLight* World::CreateRectLight() {
	return CreateEntity<TRectLight>();
}

TSimplePlane* World::CreateSimplePlane() {
	return CreateEntity<TSimplePlane>();
}

TSimpleCube* World::CreateSimpleCube() {
	return CreateEntity<TSimpleCube>();
}

TSimpleCone* World::CreateSimpleCone() {
	return CreateEntity<TSimpleCone>();
}

TSimpleSphere* World::CreateSimpleSphere() {
	return CreateEntity<TSimpleSphere>();
}

TSkybox* World::CreateSkybox(std::string path) {
	return CreateEntity<TSkybox>(path);
}

TScene* World::CreateScene(std::string path) {
	return CreateEntity<TScene>(path);
}

TAudio* World::CreateAudio(std::string path) {
	return CreateEntity<TAudio>(path);
}

std::vector<ComponentHolder*> World::GetEntities() {
	std::vector<ComponentHolder*> result;
	result.reserve(m_vEntities.size());
	for (auto& e : m_vEntities) {
		result.push_back(e.get());
	}
	return result;
}

void World::DestroyEntity(ComponentHolder* holder) {
	if (auto* renderEntity = dynamic_cast<TRender*>(holder)) {
		GlobalRenderThreadManager::GetInstance()->WaitIdle();
	}

	std::erase_if(m_vEntities, [&](const std::shared_ptr<ComponentHolder>& ent) {
		if (ent.get() == holder) {
			ent->Destroy();
			ent->SetWorld(nullptr);
			return true;
		}
		return false;
	});
}

void World::InitRenderEntity(TRender* render) {
	GlobalRenderThreadManager::GetInstance()->WaitIdle();
	auto renderInitDefaultH = render->RenderInit(m_pRender->GetDevice(), nullptr);
	if (m_pRender->IsRTSupported()) render->RenderInitDXR(m_pRender->GetDXRDevice(), renderInitDefaultH);
}

void World::AfterSetMainRenderer() {
	for (auto entity : m_vEntities) {
		entity->Init();
		if (auto* renderEntity = dynamic_cast<TRender*>(entity.get())) {
			InitRenderEntity(renderEntity);
		}
	}
}

