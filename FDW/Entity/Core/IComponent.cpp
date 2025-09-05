#include <Entity/Core/IComponent.h>
#include <Entity/Core/ComponentHolder.h>
#include <World/World.h>


void IComponent::SetOwner(ComponentHolder* owner) {
	m_pOwner = owner;
}

ComponentHolder* IComponent::GetOwner() {
	return m_pOwner;
}
World* IComponent::GetWorld() {
	return m_pOwner->GetWorld();
}


void IComponent::Init() {}

void IComponent::BeforeRenderTick(float dt) {}

void IComponent::AfterRenderTick(float dt) { }

bool IComponent::IsActive() {
	return m_bIsActive;
}

void IComponent::Activate(bool a) {
	m_bIsActive = a;
}

