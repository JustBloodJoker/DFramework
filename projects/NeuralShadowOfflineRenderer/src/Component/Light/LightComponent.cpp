#include "pch.h"

#include <Component/Light/LightComponent.h>
#include <World/World.h>

LightComponentData LightComponent::NormalizeData(LightComponentData data) {
	data.SourceRadius = std::isfinite(data.SourceRadius)
		? std::clamp(data.SourceRadius, 0.0f, LIGHT_MAX_SOURCE_RADIUS) : 0.0f;
	data.DirectionalAngularRadius = std::isfinite(data.DirectionalAngularRadius)
		? std::clamp(data.DirectionalAngularRadius, 0.0f, LIGHT_MAX_DIRECTIONAL_ANGULAR_RADIUS) : 0.0f;
	data.ShadowSoftness = std::isfinite(data.ShadowSoftness)
		? std::clamp(data.ShadowSoftness, 0.0f, LIGHT_MAX_SHADOW_SOFTNESS) : 1.0f;
	return data;
}

LightComponent::LightComponent() {
	m_sName = "LightComponent";
}

LightComponentData LightComponent::GetLightComponentData() {
	return m_xData;
}

void LightComponent::SetLightComponentData(LightComponentData newData) {
	SetLightComponentDataSilent(newData);
	GetWorld()->AddNotifyToPull(NRenderSystemNotifyType::LightUpdateData);
}

void LightComponent::SetLightComponentDataSilent(const LightComponentData& newData) {
    m_xData = NormalizeData(newData);
}

void LightComponent::Init() {
	GetWorld()->AddNotifyToPull(NRenderSystemNotifyType::Light);
}

void LightComponent::Activate(bool a) {
	IComponent::Activate(a);
	GetWorld()->AddNotifyToPull(NRenderSystemNotifyType::Light);
}
