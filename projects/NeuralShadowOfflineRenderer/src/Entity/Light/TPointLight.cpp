#include "pch.h"

#include <Entity/Light/TPointLight.h>
#include <World/World.h>

TPointLight::TPointLight() {
	m_sName = "PointLight";
}

void TPointLight::AfterCreation() {
	TLight::AfterCreation();
	m_pShadowComponent = AddComponent<ShadowComponent>();
}

NLightComponentTypes TPointLight::LightType() {
	return NLightComponentTypes::PointLight;
}

void TPointLight::SetLightPosition(dx::XMFLOAT3 position) {
	if (!m_pLightComponent) return;

	auto data = m_pLightComponent->GetLightComponentData();
	data.Position = position;
	data.LightType = static_cast<int>(LightType());
	m_pLightComponent->SetLightComponentData(data);
}

void TPointLight::SetLightSourceRadius(float sourceRadius) {
	if (!m_pLightComponent) return;

	auto data = m_pLightComponent->GetLightComponentData();
	data.SourceRadius = sourceRadius;
	data.LightType = static_cast<int>(LightType());
	m_pLightComponent->SetLightComponentData(data);
}

void TPointLight::SetLightAttenuationRadius(float attenuationRadius) {
	if (!m_pLightComponent) return;

	auto data = m_pLightComponent->GetLightComponentData();
	data.AttenuationRadius = attenuationRadius;
	data.LightType = static_cast<int>(LightType());
	m_pLightComponent->SetLightComponentData(data);
}
