#include "pch.h"

#include <Entity/Light/TSpotLight.h>
#include <World/World.h>

TSpotLight::TSpotLight() {
	m_sName = "SpotLight";
}

void TSpotLight::AfterCreation() {
	TLight::AfterCreation();
	m_pShadowComponent = AddComponent<ShadowComponent>();
}

NLightComponentTypes TSpotLight::LightType() {
	return NLightComponentTypes::SpotLight;
}

void TSpotLight::SetLightPosition(dx::XMFLOAT3 position) {
	if (!m_pLightComponent) return;

	auto data = m_pLightComponent->GetLightComponentData();
	data.Position = position;
	data.LightType = static_cast<int>(LightType());
	m_pLightComponent->SetLightComponentData(data);
}

void TSpotLight::SetLightDirection(dx::XMFLOAT3 direction) {
	if (!m_pLightComponent) return;

	auto data = m_pLightComponent->GetLightComponentData();
	data.Direction = direction;
	data.LightType = static_cast<int>(LightType());
	m_pLightComponent->SetLightComponentData(data);
}

void TSpotLight::SetLightOuterConeAngle(float angle) {
	if (!m_pLightComponent) return;

	auto data = m_pLightComponent->GetLightComponentData();
	data.OuterConeAngle = angle;
	data.LightType = static_cast<int>(LightType());
	m_pLightComponent->SetLightComponentData(data);
}

void TSpotLight::SetLightInnerConeAngle(float angle) {
	if (!m_pLightComponent) return;

	auto data = m_pLightComponent->GetLightComponentData();
	data.InnerConeAngle = angle;
	data.LightType = static_cast<int>(LightType());
	m_pLightComponent->SetLightComponentData(data);
}

void TSpotLight::SetLightAttenuationRadius(float attenuationRadius) {
	if (!m_pLightComponent) return;

	auto data = m_pLightComponent->GetLightComponentData();
	data.AttenuationRadius = attenuationRadius;
	data.LightType = static_cast<int>(LightType());
	m_pLightComponent->SetLightComponentData(data);
}

void TSpotLight::SetLightSourceRadius(float sourceRadius) {
	if (!m_pLightComponent) return;

	auto data = m_pLightComponent->GetLightComponentData();
	data.SourceRadius = sourceRadius;
	data.LightType = static_cast<int>(LightType());
	m_pLightComponent->SetLightComponentData(data);
}
