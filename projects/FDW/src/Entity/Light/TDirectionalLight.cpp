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
