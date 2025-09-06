#include <Entity/Core/ComponentHolder.h>
#include <World/World.h>

void ComponentHolder::RemoveComponent(IComponent* ptr) {
    for (auto it = m_vComponents.begin(); it != m_vComponents.end(); ++it) {
        if (it->get() == ptr) {
            HandleComponentRemoval(ptr);
            m_vComponents.erase(it);
            break;
        }
    }
}

void ComponentHolder::RemoveComponentAt(size_t index) {
    if (index < m_vComponents.size()) {
        auto& comp = m_vComponents[index];
        HandleComponentRemoval(comp.get());
        m_vComponents.erase(m_vComponents.begin() + index);
    }
}

void ComponentHolder::RemoveAllComponents() {
    for (auto& comp : m_vComponents) {
        HandleComponentRemoval(comp.get());
    }
    m_vComponents.clear();
}


bool ComponentHolder::IsActive() {
    for (auto& cmp : m_vComponents) {
        if (cmp->IsActive()) return true;
    }
    return false;
}

void ComponentHolder::Activate(bool b) {
    for (auto& cmp : m_vComponents) {
        cmp->Activate(b);
    }
}

void ComponentHolder::SetWorld(World* world) { m_pWorld = world; }

World* ComponentHolder::GetWorld() { return m_pWorld; }


const std::string& ComponentHolder::GetName() const {
	return m_sName;
}

void ComponentHolder::SetName(std::string name) {
    m_sName = name;
}

void ComponentHolder::AfterCreation() {}

void ComponentHolder::Init() {
	for (auto& cmp : m_vComponents) {
		cmp->Init();
	}
}

void ComponentHolder::Destroy() {
	RemoveAllComponents();
}

void ComponentHolder::AddNotifyToPull(NRenderSystemNotifyType type) {
	m_pWorld->AddNotifyToPull(type);
}

void ComponentHolder::HandleComponentRemoval(IComponent* comp) {
    OnComponentRemoved(comp);
    comp->Destroy();
    comp->SetOwner(nullptr);
}

void ComponentHolder::OnComponentRemoved(IComponent* comp) { }
