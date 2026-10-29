#include "pch.h"

#include <Entity/Light/TRectLight.h>
#include <World/World.h>

TRectLight::TRectLight() {
	m_sName = "TRectLight";
}

void TRectLight::AfterCreation() {
	TLight::AfterCreation();
	m_pShadowComponent = AddComponent<ShadowComponent>();
}

NLightComponentTypes TRectLight::LightType() {
	return NLightComponentTypes::RectLight;
}

void TRectLight::SetLightPosition(dx::XMFLOAT3 position) {
	if (!m_pLightComponent) return;

	auto data = m_pLightComponent->GetLightComponentData();
	data.Position = position;
	data.LightType = static_cast<int>(LightType());
	m_pLightComponent->SetLightComponentData(data);
}

void TRectLight::SetLightRotation(dx::XMFLOAT3 rotation) {
	if (!m_pLightComponent) return;

	auto data = m_pLightComponent->GetLightComponentData();
	data.Rotation = rotation;
	data.LightType = static_cast<int>(LightType());
	m_pLightComponent->SetLightComponentData(data);
}

void TRectLight::SetLightRectSize(dx::XMFLOAT2 size) {
	if (!m_pLightComponent) return;

	auto data = m_pLightComponent->GetLightComponentData();
	data.RectSize = size;
	data.LightType = static_cast<int>(LightType());
	m_pLightComponent->SetLightComponentData(data);
}
