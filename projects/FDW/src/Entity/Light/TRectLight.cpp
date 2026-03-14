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
