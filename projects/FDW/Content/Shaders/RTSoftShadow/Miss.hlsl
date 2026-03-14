#include "SameShadersStructs.hlsli"

[shader("miss")]
void Miss(inout ShadowPayload payload)
{
    payload.shadow = 1.0f;
}
