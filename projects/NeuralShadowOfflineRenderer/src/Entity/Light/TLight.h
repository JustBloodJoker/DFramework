#pragma once

#include <Entity/Core/ComponentHolder.h>
#include <Component/Light/LightComponent.h>
#include <Component/Shadow/ShadowComponent.h>

class TLight : public ComponentHolder {
public:
	TLight() = default;
	virtual ~TLight() = default;

public:
	REFLECT_BODY(TLight)
	BEGIN_REFLECT(TLight, ComponentHolder)
		REFLECT_PROPERTY(m_pLightComponent)
		REFLECT_PROPERTY(m_pShadowComponent)
	END_REFLECT(TLight)

public:
	virtual void AfterCreation() override;
	virtual NLightComponentTypes LightType() = 0;

	void ActivateLightComponent(bool active);
	void ActivateShadowComponent(bool active);
	void SetLightIntensity(float intensity);
	void SetLightColor(dx::XMFLOAT3 color);
	void SetLightShadowSoftness(float softness);

protected:
	LightComponent* m_pLightComponent = nullptr;
	ShadowComponent* m_pShadowComponent = nullptr;
};
