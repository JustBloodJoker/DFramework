#include <Entity/Light/TLight.h>
#include <World/World.h>


void TLight::AfterCreation() {
	ComponentHolder::AfterCreation();

	m_pLightComponent = AddComponent<LightComponent>();
	auto data = m_pLightComponent->GetLightComponentData();
	data.LightType = int(LightType());
	m_pLightComponent->SetLightComponentData(data);
}

void TLight::OnComponentRemoved(IComponent* comp) {
	if (comp == m_pLightComponent) m_pLightComponent = nullptr;
	if (comp == m_pShadowComponent) m_pShadowComponent = nullptr;
}

bool TLight::IsHaveShadowComponent() {
	return m_pShadowComponent != nullptr;
}

bool TLight::IsHaveLightComponent() {
	return m_pLightComponent != nullptr;
}

void TLight::ActivateLightComponent(bool b) {
	if (m_pLightComponent) m_pLightComponent->Activate(b);
}

bool TLight::IsActiveLightComponent() {
	return m_pLightComponent ? m_pLightComponent->IsActive() : false;
}

void TLight::ActivateShadowComponent(bool b) {
	if (m_pShadowComponent) m_pShadowComponent->Activate(b);
}

bool TLight::IsActiveShadowComponent() {
	return m_pShadowComponent ? m_pShadowComponent->IsActive() : false;
}

LightComponentData TLight::GetLightData() const {
	return m_pLightComponent ? m_pLightComponent->GetLightComponentData() : LightComponentData({});
}

ShadowComponentData TLight::GetShadowData() const {
	return m_pShadowComponent ? m_pShadowComponent->GetShadowComponentData() : ShadowComponentData({});
}