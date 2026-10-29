#pragma once

#include <Entity/Light/TLight.h>

class TDirectionalLight : public TLight {
public:
	TDirectionalLight();
	virtual ~TDirectionalLight()=default;

public:

    REFLECT_BODY(TDirectionalLight)
    BEGIN_REFLECT(TDirectionalLight, TLight)
    END_REFLECT(TDirectionalLight)

public:
	virtual void AfterCreation() override;
	virtual NLightComponentTypes LightType() override;

	void SetLightDirection(dx::XMFLOAT3 direction);
	void SetLightDirectionalAngularRadius(float angularRadius);

};
