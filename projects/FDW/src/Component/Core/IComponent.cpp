#include <Component/Core/IComponent.h>
#include <Entity/Core/ComponentHolder.h>
#include <World/World.h>


void IComponent::SetOwner(ComponentHolder* owner) {
	m_pOwner = owner;
}

ComponentHolder* IComponent::GetOwner() {
	return m_pOwner;
}
World* IComponent::GetWorld() {
	return m_pOwner ? m_pOwner->GetWorld() : nullptr;
}

std::string IComponent::GetName() const {
	return m_sName;
}

void IComponent::SetName(std::string name) {
	m_sName = name;
}

void IComponent::Init() {}

bool IComponent::IsActive() {
	return m_bIsActive;
}

void IComponent::Activate(bool a) {
	m_bIsActive = a;
}

