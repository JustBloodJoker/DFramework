#include <Entity/Light/ShadowComponent.h>
#include <World/World.h>

ShadowComponentData ShadowComponent::GetShadowComponentData() {
	return m_xData;
}

void ShadowComponent::SetShadowComponentData(ShadowComponentData newData) {
	m_xData = newData;
	GetWorld()->AddNotifyToPull(NRenderSystemNotifyType::Shadow);
}

void ShadowComponent::Init() {
	GetWorld()->AddNotifyToPull(NRenderSystemNotifyType::Shadow);
}

void ShadowComponent::Activate(bool a) {
	IComponent::Activate(a);
	GetWorld()->AddNotifyToPull(NRenderSystemNotifyType::Shadow);
}

void ShadowComponent::Destroy() {
	GetWorld()->AddNotifyToPull(NRenderSystemNotifyType::Shadow);
}
