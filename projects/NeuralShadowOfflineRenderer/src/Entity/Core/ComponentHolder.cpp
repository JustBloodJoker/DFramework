#include "pch.h"

#include <Entity/Core/ComponentHolder.h>
#include <World/World.h>

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


void ComponentHolder::AfterCreation() {}

void ComponentHolder::Init() {
	for (auto& cmp : m_vComponents) {
		cmp->Init();
	}
}

void ComponentHolder::AddNotifyToPull(NRenderSystemNotifyType type) {
	m_pWorld->AddNotifyToPull(type);
}
