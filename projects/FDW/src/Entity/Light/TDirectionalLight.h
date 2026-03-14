#pragma once

#include <pch.h>
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

	IMPL_VALUE_GET_SET_FOR_LIGHT_ENTITY(Direction, Light, dx::XMFLOAT3);

};
