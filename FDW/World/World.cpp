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

void World::DestroyEntity(ComponentHolder* holder) {
	std::erase_if(m_vEntities, [&](const std::shared_ptr<ComponentHolder>& ent) {
		if (ent.get() == holder) {
			ent->Destroy();
			ent->SetWorld(nullptr);
			return true;
		}
		return false;
	});
}

void World::AfterSetMainRenderer() {
	for (auto entity : m_vEntities) {
		entity->Init();
	}
}

