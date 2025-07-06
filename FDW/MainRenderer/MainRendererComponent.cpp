#include <MainRenderer/MainRendererComponent.h>
#include <MainRenderer/MainRenderer.h>

MainRendererComponent::MainRendererComponent(MainRenderer* owner) {
	m_pOwner = owner;
}

MainRenderer* MainRendererComponent::Owner() {
	return m_pOwner;
}

void MainRendererComponent::AfterConstruction() { }

void MainRendererComponent::BeforeDestruction() { }