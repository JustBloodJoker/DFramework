#pragma once

#include <Entity/Light/TLight.h>

class TSpotLight : public TLight {
public:
	TSpotLight();
	virtual ~TSpotLight() = default;

public:

    REFLECT_BODY(TSpotLight)
    BEGIN_REFLECT(TSpotLight, TLight)
    END_REFLECT(TSpotLight)

public:
	virtual void AfterCreation() override;
	virtual NLightComponentTypes LightType() override;

	void SetLightPosition(dx::XMFLOAT3 position);
	void SetLightDirection(dx::XMFLOAT3 direction);
	void SetLightOuterConeAngle(float angle);
	void SetLightInnerConeAngle(float angle);
	void SetLightAttenuationRadius(float attenuationRadius);
	void SetLightSourceRadius(float sourceRadius);
};
