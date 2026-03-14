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
