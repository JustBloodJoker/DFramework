#pragma once

#include <pch.h>
#include <Entity/Light/TLight.h>

class TRectLight : public TLight {
public:
	TRectLight();
	virtual ~TRectLight() = default;

public:

	BEGIN_FIELD_REGISTRATION(TRectLight, TLight)
	END_FIELD_REGISTRATION();

public:
	virtual void AfterCreation() override;
	virtual NLightComponentTypes LightType() override;

	IMPL_VALUE_GET_SET_FOR_LIGHT_ENTITY(Position, Light, dx::XMFLOAT3);
	IMPL_VALUE_GET_SET_FOR_LIGHT_ENTITY(Rotation, Light, dx::XMFLOAT3);
	IMPL_VALUE_GET_SET_FOR_LIGHT_ENTITY(RectSize, Light, dx::XMFLOAT2);
};
