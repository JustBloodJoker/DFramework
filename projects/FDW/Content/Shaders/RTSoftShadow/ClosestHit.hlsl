#include "SameShadersStructs.hlsli"

struct Attributes {
    float2 bary;
};

[shader("closesthit")]
void ClosestHit(inout ShadowPayload payload, Attributes attr)
{
    payload.shadow = 0.0f;
}
