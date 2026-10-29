#include "pch.h"

#include <Entity/Light/TDirectionalLight.h>
#include <World/World.h>


TDirectionalLight::TDirectionalLight() {
	m_sName = "DirectionalLight";
}

void TDirectionalLight::AfterCreation() {
	TLight::AfterCreation();
	m_pShadowComponent = AddComponent<ShadowComponent>();
}

NLightComponentTypes TDirectionalLight::LightType() {
	return NLightComponentTypes::DirectionalLight;
}

void TDirectionalLight::SetLightDirection(dx::XMFLOAT3 direction) {
	if (!m_pLightComponent) return;

	auto data = m_pLightComponent->GetLightComponentData();
	data.Direction = direction;
	data.LightType = static_cast<int>(LightType());
	m_pLightComponent->SetLightComponentData(data);
}

void TDirectionalLight::SetLightDirectionalAngularRadius(float angularRadius) {
	if (!m_pLightComponent) return;

	auto data = m_pLightComponent->GetLightComponentData();
	data.DirectionalAngularRadius = angularRadius;
	data.LightType = static_cast<int>(LightType());
	m_pLightComponent->SetLightComponentData(data);
}
