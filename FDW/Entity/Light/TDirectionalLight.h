#pragma once

#include <pch.h>
#include <Entity/Light/TLight.h>

class TDirectionalLight : public TLight {
public:
	TDirectionalLight();
	virtual ~TDirectionalLight()=default;
	
public:

	BEGIN_FIELD_REGISTRATION(TDirectionalLight, TLight)
	END_FIELD_REGISTRATION();

public:
	virtual void AfterCreation() override;
	virtual NLightComponentTypes LightType() override;

	IMPL_VALUE_GET_SET_FOR_LIGHT_ENTITY(Direction, Light, dx::XMFLOAT3);

};
