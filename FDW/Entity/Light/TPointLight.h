#pragma once

#include <pch.h>
#include <Entity/Light/TLight.h>

class TPointLight : public TLight {
public:
	TPointLight();
	virtual ~TPointLight() = default;

public:

	BEGIN_FIELD_REGISTRATION(TPointLight, TLight)
	END_FIELD_REGISTRATION();

public:
	virtual void AfterCreation() override;
	virtual NLightComponentTypes LightType() override;

	IMPL_VALUE_GET_SET_FOR_LIGHT_ENTITY(Position, Light, dx::XMFLOAT3);
	IMPL_VALUE_GET_SET_FOR_LIGHT_ENTITY(SourceRadius, Light, float);
	IMPL_VALUE_GET_SET_FOR_LIGHT_ENTITY(AttenuationRadius, Light, float);

};
