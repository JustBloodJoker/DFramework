#pragma once

#include <pch.h>
#include <Entity/Light/TLight.h>

class TSpotLight : public TLight {
public:
	TSpotLight();
	virtual ~TSpotLight() = default;

public:

	BEGIN_FIELD_REGISTRATION(TSpotLight, TLight)
	END_FIELD_REGISTRATION();

public:
	virtual void AfterCreation() override;
	virtual NLightComponentTypes LightType() override;

	IMPL_VALUE_GET_SET_FOR_LIGHT_ENTITY(Position, Light, dx::XMFLOAT3);
	IMPL_VALUE_GET_SET_FOR_LIGHT_ENTITY(Direction, Light, dx::XMFLOAT3);
	IMPL_VALUE_GET_SET_FOR_LIGHT_ENTITY(OuterConeAngle, Light, float);
	IMPL_VALUE_GET_SET_FOR_LIGHT_ENTITY(InnerConeAngle, Light, float);
	IMPL_VALUE_GET_SET_FOR_LIGHT_ENTITY(AttenuationRadius, Light, float);
};
