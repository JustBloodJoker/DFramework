#pragma once

#include <Entity/Light/TLight.h>

class TRectLight : public TLight {
public:
	TRectLight();
	virtual ~TRectLight() = default;

public:

    REFLECT_BODY(TRectLight)
    BEGIN_REFLECT(TRectLight, TLight)
    END_REFLECT(TRectLight)

public:
	virtual void AfterCreation() override;
	virtual NLightComponentTypes LightType() override;

	void SetLightPosition(dx::XMFLOAT3 position);
	void SetLightRotation(dx::XMFLOAT3 rotation);
	void SetLightRectSize(dx::XMFLOAT2 size);
};
