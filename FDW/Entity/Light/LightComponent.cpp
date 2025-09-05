#include <Entity/Light/LightComponent.h>
#include <World/World.h>


LightComponentData LightComponent::GetLightComponentData() {
	return m_xData;
}

void LightComponent::SetLightComponentData(LightComponentData newData) {
	m_xData = newData;
	GetWorld()->AddNotifyToPull(NRenderSystemNotifyType::Light);
}

void LightComponent::Init() {
	GetWorld()->AddNotifyToPull(NRenderSystemNotifyType::Light);
}

void LightComponent::Activate(bool a) {
	IComponent::Activate(a);
	GetWorld()->AddNotifyToPull(NRenderSystemNotifyType::Light);
}

void LightComponent::Destroy() {
	GetWorld()->AddNotifyToPull(NRenderSystemNotifyType::Light);
}
