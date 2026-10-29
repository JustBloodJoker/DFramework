#pragma once

#include <Entity/Light/TLight.h>

class TPointLight : public TLight {
public:
	TPointLight();
	virtual ~TPointLight() = default;

public:

    REFLECT_BODY(TPointLight)
    BEGIN_REFLECT(TPointLight, TLight)
    END_REFLECT(TPointLight)

public:
	virtual void AfterCreation() override;
	virtual NLightComponentTypes LightType() override;

	void SetLightPosition(dx::XMFLOAT3 position);
	void SetLightSourceRadius(float sourceRadius);
	void SetLightAttenuationRadius(float attenuationRadius);

};
