#include <MainRenderer/MainRendererComponent.h>
#include <MainRenderer/MainRenderer.h>

MainRenderer* MainRendererComponent::Owner() {
	return m_pOwner;
}

void MainRendererComponent::ProcessNotify(NRenderSystemNotifyType type) {
}
World* MainRendererComponent::GetWorld() { return m_pOwner->GetWorld(); }

void MainRendererComponent::AfterConstruction() {}

void MainRendererComponent::SetAfterConstruction(MainRenderer* owner) {
	m_pOwner = owner;
	AfterConstruction();
}

void MainRendererComponent::BeforeDestruction() { }