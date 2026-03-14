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
