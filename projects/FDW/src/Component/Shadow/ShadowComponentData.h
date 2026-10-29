#pragma once

#include <WinWindow/Utils/Reflection/Reflection.h>

struct ShadowComponentData {
	bool CastsShadows = true;
	float Priority = 0.0f;

    REFLECT_STRUCT(ShadowComponentData)
    BEGIN_REFLECT(ShadowComponentData)
		REFLECT_PROPERTY(CastsShadows)
		REFLECT_PROPERTY(Priority)
    END_REFLECT(ShadowComponentData)
};
