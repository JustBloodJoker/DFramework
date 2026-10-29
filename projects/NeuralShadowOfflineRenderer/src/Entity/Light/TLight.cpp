#include "pch.h"

#include <Entity/Light/TLight.h>
#include <World/World.h>

void TLight::AfterCreation() {
	ComponentHolder::AfterCreation();

	m_pLightComponent = AddComponent<LightComponent>();
	auto data = m_pLightComponent->GetLightComponentData();
	data.LightType = static_cast<int>(LightType());
	m_pLightComponent->SetLightComponentData(data);
}

void TLight::ActivateLightComponent(bool active) {
	if (m_pLightComponent) m_pLightComponent->Activate(active);
}

void TLight::ActivateShadowComponent(bool active) {
	if (m_pShadowComponent) m_pShadowComponent->Activate(active);
}

void TLight::SetLightIntensity(float intensity) {
	if (!m_pLightComponent) return;

	auto data = m_pLightComponent->GetLightComponentData();
	data.Intensity = intensity;
	data.LightType = static_cast<int>(LightType());
	m_pLightComponent->SetLightComponentData(data);
}

void TLight::SetLightColor(dx::XMFLOAT3 color) {
	if (!m_pLightComponent) return;

	auto data = m_pLightComponent->GetLightComponentData();
	data.Color = color;
	data.LightType = static_cast<int>(LightType());
	m_pLightComponent->SetLightComponentData(data);
}

void TLight::SetLightShadowSoftness(float softness) {
	if (!m_pLightComponent) return;

	auto data = m_pLightComponent->GetLightComponentData();
	data.ShadowSoftness = softness;
	data.LightType = static_cast<int>(LightType());
	m_pLightComponent->SetLightComponentData(data);
}
