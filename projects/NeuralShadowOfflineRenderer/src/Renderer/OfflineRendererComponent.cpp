#include "pch.h"

#include <Renderer/OfflineRendererComponent.h>
#include <Renderer/OfflineRenderer.h>

void OfflineRendererComponent::ProcessNotify(NRenderSystemNotifyType type) {}

World* OfflineRendererComponent::GetWorld() { return m_pOwner->GetWorld(); }

void OfflineRendererComponent::AfterConstruction() {}

void OfflineRendererComponent::SetAfterConstruction(OfflineRenderer* owner) {
	m_pOwner = owner;
	AfterConstruction();
}
